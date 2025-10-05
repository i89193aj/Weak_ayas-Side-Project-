#include "ThreadPool.hpp"
#include "EpollWorker.hpp"
#include "TCPServer.hpp"   

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace threadpool;
using namespace epollworker;

namespace tcpserver {
TCPServer::TCPServer(int port, ThreadPool& sharedPool, size_t nWorkers, std::atomic<bool>* stopAllPtr, std::mutex& extern_mutex, std::condition_variable& extern_cv)
    : port(port), running(false), next_worker(0), pool(sharedPool), stop_all(stopAllPtr), extern_mutexptr(&extern_mutex), extern_cvptr(&extern_cv)
{
    // 建立 EpollWorker
    for (size_t i = 0; i < nWorkers; ++i) {
        workers.emplace_back(std::make_unique<EpollWorker>(pool, stop_all));
    }
}

TCPServer::~TCPServer() {
    stop(); // 析構時確保安全關閉
	std::cout << "TCPServer is stoped!" << std::endl;
}

bool TCPServer::start() {
    if (running) return false;

    // 建立 TCP socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) { perror("socket"); return false; }
	
	// 新增：允許快速重複綁定同一個 port（解決 TIME_WAIT 問題）
	int opt = 1;
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt SO_REUSEADDR");
		return false;
	}

    // 設定 server address
    sockaddr_in serveraddr{};
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY; // 監聽所有 IP
    serveraddr.sin_port = htons(port);

    // 綁定 IP + Port
    if (bind(server_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind");
        return false;
    }

    // 開始監聽
    if (listen(server_sock, 128) < 0) {
        perror("listen");
        return false;
    }

    // 設為非阻塞
    set_nonblocking(server_sock);

    // 建立 epoll 監控 server_sock
    accept_epoll_fd = epoll_create1(0);
    if (accept_epoll_fd < 0) { perror("epoll_create1"); return false; }

    epoll_event ev{};
    ev.events = EPOLLIN;      // 只監聽可讀事件
    ev.data.fd = server_sock;
    if (epoll_ctl(accept_epoll_fd, EPOLL_CTL_ADD, server_sock, &ev) != 0) {
        perror("epoll_ctl ADD server_sock");
        return false;
    }

    // 建立 eventfd，用來通知 accept_loop 關閉
    notify_fd = eventfd(0, EFD_NONBLOCK);
    if (notify_fd < 0) { perror("notify_fd create"); return false; }

    epoll_event ev_notify{};
    ev_notify.events = EPOLLIN;
    ev_notify.data.fd = notify_fd;
    if (epoll_ctl(accept_epoll_fd, EPOLL_CTL_ADD, notify_fd, &ev_notify) != 0) {
        perror("epoll_ctl ADD notify_fd");
        return false;
    }
	
    // 啟動 accept thread
	running = true;
    accept_thread = std::thread([this]{ accept_loop(); });

    std::cout << "Server listening on port " << port << std::endl;
    return true;
}

void TCPServer::stop() {
    if(close_all.load()) return;
	close_all.store(true);
    running = false;

    // 通知 accept_loop 退出
    if (notify_fd >= 0) {
        uint64_t one = 1;
        ssize_t n = write(notify_fd, &one, sizeof(one));
		if (n != sizeof(one)) 
			perror("write notify_fd");
    }
	
	std::cout << "Notify Server End" << std::endl;
    if (accept_thread.joinable())
        accept_thread.join();
		std::cout << "accept_thread is stoped" << std::endl;
	
	//通知 client，已不在被Server 接收
	{
        std::lock_guard<std::mutex> lk(clients_mutex); 
        for (int fd : client_sockets) {                
            shutdown(fd, SHUT_WR);                     
            close(fd);                                 
        }
        client_sockets.clear(); 
		std::cout << "Server let all clients are closed!" << std::endl;		
    }

    // 關閉 server socket
    if (server_sock >= 0){
		close(server_sock);
		std::cout << "close->server_sock" << std::endl;
	}
	
	// 通知 accept_loop 退出
    if (notify_fd >= 0) {
        close(notify_fd);//這裡不要馬上刪除!
        std::cout << "close->notify_fd" << std::endl;
    }

    // 關閉 epoll fd
    if (accept_epoll_fd >= 0){
		close(accept_epoll_fd);
		std::cout << "close->accept_epoll_fd" << std::endl;
	}
	
    // 清理 EpollWorker
    workers.clear();
}

// 取得 Server IP
std::string TCPServer::get_ip() const {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getsockname(server_sock, (sockaddr*)&addr, &len) == 0) {
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));
        return std::string(buf);
    }
    return "";
}

// 取得 Server Port
int TCPServer::get_port() const {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getsockname(server_sock, (sockaddr*)&addr, &len) == 0)
        return ntohs(addr.sin_port);
    return -1;
}

// 設定 fd 為非阻塞
void TCPServer::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) { perror("fcntl F_GETFL"); return; }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        perror("fcntl F_SETFL");
}

// accept loop，處理新 client 連線
void TCPServer::accept_loop() {
    const int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];

    while (running && !*stop_all) {
        int n = epoll_wait(accept_epoll_fd, events, MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait accept_epoll_fd");
			notify_false();
            break;
        }

        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == server_sock) {
                // 接收新 client 連線
                while (true) {
                    sockaddr_in client_addr{};
                    socklen_t len = sizeof(client_addr);
                    int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &len);
                    if (client_sock < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break; // 非阻塞下沒資料
                        perror("accept");
                        break;
                    }

                    set_nonblocking(client_sock);                  // client socket 設為非阻塞
                    workers[next_worker]->add_fd(client_sock);    // 均勻分配給 EpollWorker
                    next_worker = (next_worker + 1) % workers.size();
					
					// 記錄 client socket fd
                    {
                        std::lock_guard<std::mutex> lk(clients_mutex);
                        client_sockets.push_back(client_sock);
                    }
                }
            }
            else if (events[i].data.fd == notify_fd) {
                uint64_t val;
                read(notify_fd, &val, sizeof(val)); // 清掉 eventfd
                running = false;                     // 收到通知，安全退出 loop
                break;
            }
        }
    }
}

void TCPServer::notify_false(){
    if (extern_mutexptr && extern_cvptr) {
        std::unique_lock<std::mutex> lock(*extern_mutexptr);
        running = false;
        extern_cvptr->notify_one();
    }
}
	
bool TCPServer::get_serverstats(){
    return running;
}
}