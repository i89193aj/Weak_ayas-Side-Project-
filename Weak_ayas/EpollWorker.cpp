#include "EpollWorker.hpp"
#include "ThreadPool.hpp"
#include "SQLManager.hpp"

using namespace threadpool;
using namespace sqlmanager;
using namespace protocol;
using namespace protocol::sql_protocol;

namespace epollworker {

EpollWorker::EpollWorker(ThreadPool &pool, std::atomic<bool>* stop_all_ptr)
    : pool(pool), stop_all(stop_all_ptr)
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1 failed");
        exit(1);
    }
	
	running.store(true);
    if (pthread_create(&worker_thread, nullptr, &EpollWorker::threadFunc, this) != 0) {
        perror("pthread_create failed");
        exit(1);
    }
}

EpollWorker::~EpollWorker() {
	//*stop_all.store(false); 這是給外部控制的，不能在這裡!
    if (!running.load()) {
        std::cout << "[EpollWorker] Already stopped, skipping cleanup." << std::endl;
    } else {
        running.store(false);
        std::cout << "[EpollWorker] Stopping worker..." << std::endl;

        pthread_join(worker_thread, nullptr);

        for (auto& [fd, info] : fd_infos) {
            std::unique_lock<std::mutex> lock(*info->fd_mutex);
				info->fd_cond.wait(lock, [&info] { 
					return !info->thread_running.load(); 
				}
			);
        }
    }

    if (epoll_fd >= 0) {
        close(epoll_fd);
        epoll_fd = -1;
    }

    std::cout << "[EpollWorker] Cleaned up successfully!" << std::endl;
}

void EpollWorker::set_nonblocking(int fd) const {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) { perror("fcntl F_GETFL failed"); return; }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) { perror("fcntl F_SETFL failed"); }
}

void EpollWorker::add_fd(int fd) {
    set_nonblocking(fd);
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) != 0) {
        perror("epoll_ctl ADD failed");
        return;
    }

    std::lock_guard<std::mutex> lk(fd_map);
    fd_infos[fd] = std::make_shared<fd_info>();
}

bool EpollWorker::get_lock_fdinfo(std::shared_ptr<fd_info>& info_ptr, int fd) {
    std::lock_guard<std::mutex> lk(fd_map);
    auto it = fd_infos.find(fd);
    if (it == fd_infos.end()) return false;
    info_ptr = it->second;
    return true;
}

// static wrapper for ThreadPool
void EpollWorker::enqueue_event_wrapper(void* arg) {
    auto wrapper = static_cast<EventWrapper*>(arg);
    wrapper->self->enqueue_event_wrapper_nonstatic(wrapper->e);
    delete wrapper;
}

void EpollWorker::enqueue_event_wrapper_nonstatic(Event* e) {
    int fd = e->fd;
    std::shared_ptr<fd_info> info_ptr;
    if(!get_lock_fdinfo(info_ptr, fd)) { 
		delete e; 
		return; 
	}

    std::lock_guard<std::mutex> lk(*info_ptr->fd_mutex);

	//解析封包
	SocketPackage pkg;
    if (!parse_sockpackage(e->data, pkg)) {
        std::cerr << "Invalid package from fd " << fd << std::endl;
        delete e;
        return;
    }
		
	// 到這裡 pkg 已經解析完成，進入 SQLManager 處理事件
	mgr_SQL.handle_request(fd, pkg);
	
    info_ptr->fd_inthreadpool = false;
    info_ptr->fd_cond.notify_one();

    delete e;
}

void EpollWorker::active_enqueue_event(Event* e) {
    int fd = e->fd;
    std::shared_ptr<fd_info> info_ptr;
    if(!get_lock_fdinfo(info_ptr, fd)) { 
		delete e; 
		return; 
	}
	
    std::lock_guard<std::mutex> lk(*info_ptr->fd_mutex);
	//解析封包
	SocketPackage pkg;
    if (!parse_sockpackage(e->data, pkg)) {
        std::cerr << "Invalid package from fd " << fd << std::endl;
        delete e;
        return;
    }
	
	// 到這裡 pkg 已經解析完成，進入 SQLManager 處理事件
	mgr_SQL.handle_request(fd, pkg);

    delete e;
}

void* EpollWorker::threadFunc(void* arg) {
    EpollWorker* self = static_cast<EpollWorker*>(arg);
    self->loop();
    self->drain_epoll();
    return nullptr;
}

void EpollWorker::loop() {
    const int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];

    while(running && !(*stop_all)){
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 100);
        if(n < 0){
            if(errno == EINTR){
                //std::cout << "errno == EINTR" << std::endl;
                continue;
            }
            perror("epoll_wait failed");
            break;
        }
		
		//std::cout<< "log - 1" <<std::endl;
        for(int i=0; i<n; ++i){
            int fd = events[i].data.fd;
            std::shared_ptr<fd_info> info_ptr;
            if(!get_lock_fdinfo(info_ptr, fd)) continue;

			{
				//這裡鎖住，為了判別活躍thread的競爭
				std::lock_guard<std::mutex> lk(fd_map);
				if(info_ptr->thread_running){
					info_ptr->needactive.store(true);
					continue;
				} else if(info_ptr->fd_inthreadpool){
					info_ptr->needactive.store(true);
					info_ptr->thread_running.store(true);
					info_ptr->fd_thread = std::thread([this, fd]{
						std::cout<< "Active: " << fd << std::endl;
						std::shared_ptr<fd_info> info_ptr;
						if(!get_lock_fdinfo(info_ptr, fd)) return;

						while(true){

							//std::lock_guard<std::mutex> lk(*info_ptr->fd_mutex); //原子操作 不需要!
							info_ptr->needactive.store(false);			

							if(!activefd_handle_event(fd)){
								std::cout<< "close_fd: " << fd << std::endl;
								mgr_SQL.erase_fd(fd);
								close_fd(fd);
								return;
							}
							
							std::this_thread::sleep_for(std::chrono::milliseconds(10));
							
							//這裡要鎖 因為fd_cond.notify_one() 需要先把~EpollWorker()，這邊執行完在呼叫~EpollWorker()!
							std::lock_guard<std::mutex> lk(*info_ptr->fd_mutex); 
							if(!info_ptr->needactive.load()){
								info_ptr->thread_running.store(false);
								info_ptr->fd_cond.notify_one();
								return;
							}
						}
					});
					info_ptr->fd_thread.detach();
					continue;
				}
			}
			//std::cout<< "log - handle_event" <<std::endl;
            handle_event(fd);
        }
    }
}

void EpollWorker::drain_epoll() {
    const int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];
	std::cout<< "drain_epoll" <<std::endl;
    while(true){
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 0);
        if(n <= 0) break;

        for(int i=0; i<n; ++i){
            int fd = events[i].data.fd;
            std::shared_ptr<fd_info> info_ptr;
            if(!get_lock_fdinfo(info_ptr, fd)) continue;
			{
				//這裡鎖住，為了判別活躍thread的競爭
				std::lock_guard<std::mutex> lk(fd_map);
				
				if(info_ptr->thread_running){
					info_ptr->needactive.store(true);
					continue;
				} else if(info_ptr->fd_inthreadpool){
					info_ptr->needactive.store(true);
					info_ptr->thread_running.store(true);
					info_ptr->fd_thread = std::thread([this, fd]{
						std::shared_ptr<fd_info> info_ptr;
						if(!get_lock_fdinfo(info_ptr, fd)) return;

						while(true){
							//std::lock_guard<std::mutex> lk(*info_ptr->fd_mutex); 原子操作 不需要!
							info_ptr->needactive.store(false);

							if(!activefd_handle_event(fd)){
								mgr_SQL.erase_fd(fd);
								close_fd(fd);
								return;
							}
							
							std::this_thread::sleep_for(std::chrono::milliseconds(10));
																	
							//這裡要鎖 因為fd_cond.notify_one() 需要先把~EpollWorker()，這邊執行完在呼叫~EpollWorker()!
							std::lock_guard<std::mutex> lk(*info_ptr->fd_mutex);
							if(!info_ptr->needactive.load()){
								info_ptr->thread_running.store(false);
								info_ptr->fd_cond.notify_one();
								return;
							}
						}
					});
					info_ptr->fd_thread.detach();
					continue;
				}
			}
            handle_event(fd);
        }
    }
}

void EpollWorker::handle_event(int fd) {
    std::shared_ptr<fd_info> info_ptr;
    if(!get_lock_fdinfo(info_ptr, fd)) return;
	//std::cout<< "normal fd_handle: start with " << fd <<std::endl;
    char buf[MAX_BUFFERSIZE];
    int nbytes = read(fd, buf, sizeof(buf));
	//std::cout<< "is read" << nbytes << std::endl;
    if(nbytes > 0){
        {
            std::lock_guard<std::mutex> lk(*info_ptr->fd_mutex);
            info_ptr->fd_inthreadpool = true;
        }
		//std::cout<< "log - pool.enqueue" <<std::endl;
        Event* e = new Event{fd, std::string(buf,nbytes)};
        auto wrapper = new EventWrapper{this, e};
        pool.enqueue(&EpollWorker::enqueue_event_wrapper, wrapper);
    } else if(nbytes == 0){
		#if defined(RELEASE)
		mgr_SQL.erase_fd(fd);
		#endif 
        close_fd(fd);
		std::cout<< "Normal close_fd: " << fd << std::endl;
    } else {
        if(errno == EINTR) { handle_event(fd); return; }
		if(errno == EAGAIN || errno == EWOULDBLOCK) return;
		#if defined(RELEASE)
		mgr_SQL.erase_fd(fd);
		#endif 
        close_fd(fd);
		std::cout<< "Normal close_fd - 1: " << fd << "，nbytes: "<< nbytes << std::endl;
    }
}

bool EpollWorker::activefd_handle_event(int fd){
    std::shared_ptr<fd_info> info_ptr;
    if(!get_lock_fdinfo(info_ptr, fd)) return false;
	//std::cout<< "activefd_handle: start with " << fd <<std::endl;
    std::unique_lock<std::mutex> lock(*info_ptr->fd_mutex);
    info_ptr->fd_cond.wait(lock, [info_ptr]{ return !info_ptr->fd_inthreadpool; });
	//std::cout<< "active 沒被鎖住! " << fd <<std::endl;
    char buf[MAX_BUFFERSIZE];
    while(true){
        int nbytes = read(fd, buf, sizeof(buf));
        if(nbytes > 0){
            Event* e = new Event{fd, std::string(buf,nbytes)};
            active_enqueue_event(e);
        } else if(nbytes == 0){
			std::cout << "active close - 1 " << fd << std::endl;
            return false;
        } else {
            if(errno == EINTR) continue;
            if(errno == EAGAIN || errno == EWOULDBLOCK) break;
			std::cout << "active close - 2 " << fd << std::endl;
            return false;
        }
    }
    return true;
}

bool EpollWorker::parse_sockpackage(const std::string& data, SocketPackage& pkg) {
	const size_t min_pkg_size = 1 + 1 + 4 + 1; // StartByte + Type + nLen + EndByte
    if (data.size() < min_pkg_size) return false;

    // 從頭掃描 StartByte
    size_t start_pos = std::string::npos;
    for (size_t i = 0; i <= data.size() - min_pkg_size; ++i) {
        if (data[i] == protocol::PACKAGE_START_BYTE) {
            start_pos = i;
            break;
        }
    }
    if (start_pos == std::string::npos) return false; // 沒找到 StartByte

    // 至少要能讀到 Type + nLen + EndByte (start_pos + 1 + 4 = PACKAGE_END_BYTE 位置)
    if (start_pos + 1 + 4 >= data.size()) return false;

    size_t offset = start_pos;
    pkg.cStartByte = data[offset++];
    pkg.cType = data[offset++];

    // 讀 nLen (統一 big-endian)
    int32_t nLen = (static_cast<uint8_t>(data[offset])   << 24) |
				   (static_cast<uint8_t>(data[offset+1]) << 16) |
				   (static_cast<uint8_t>(data[offset+2]) << 8)  |
				   (static_cast<uint8_t>(data[offset+3]));


    offset += sizeof(int32_t);
    pkg.nLen = nLen;

    if (pkg.nLen > MAX_BUFFERSIZE) return false; // 超過緩衝區長度

    // 確認後面還有足夠空間給資料 + EndByte (offset + pkg.nLen = PACKAGE_END_BYTE 位置)
    if (offset + pkg.nLen >= data.size()) return false;

    // 讀資料
    memcpy(pkg.pDataBuf.data(), data.data() + offset, pkg.nLen);
    offset += pkg.nLen;

    // EndByte
    pkg.cEndByte = data[offset];
    if (pkg.cEndByte != protocol::PACKAGE_END_BYTE) return false;

    return true;
}


void EpollWorker::close_fd(int fd){	
    std::lock_guard<std::mutex> lk(fd_map);
    fd_infos.erase(fd);
    close(fd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}

} // namespace epollworker
