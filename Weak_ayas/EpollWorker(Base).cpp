#include <sys/epoll.h>   // epoll_create1, epoll_ctl, epoll_event, EPOLLIN
#include <pthread.h>     // pthread_t, pthread_create, pthread_join
#include <atomic>        // std::atomic
#include <string>        // std::string
#include <fcntl.h>       // fcntl, O_NONBLOCK
#include <unistd.h>      // read, close
#include <errno.h>       // errno, EAGAIN, EWOULDBLOCK
#include <memory>        // std::unique_ptr, std::make_unique
#include <iostream>      // std::cout, perror

#include "ThreadPool.hpp"
#include "EpollWorker.hpp"
using namespace threadpool;
namespace epollworker {
EpollWorker::EpollWorker(ThreadPool &pool, std::atomic<bool>* stop_all_ptr)
    : pool(pool), stop_all(stop_all_ptr)
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1 failed");
        exit(1);
    }

    if (pthread_create(&worker_thread, nullptr, &EpollWorker::threadFunc, this) != 0) {
        perror("pthread_create failed");
        exit(1);
    }
}

EpollWorker::~EpollWorker() {
    pthread_join(worker_thread, nullptr);
    close(epoll_fd);
}

void EpollWorker::set_nonblocking(int fd) const {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl F_GETFL failed");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL failed");
    }
}

void EpollWorker::add_fd(int fd) {
    set_nonblocking(fd);
    epoll_event ev{};
    ev.events = EPOLLIN; // 水平觸發
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) != 0) {
        perror("epoll_ctl ADD failed");
    }
}

//把thread 的 Loop task 包起來 (因為pthread 只接受 void* func, 跟 void * arg)
void* EpollWorker::threadFunc(void* arg) {
    EpollWorker* self = static_cast<EpollWorker*>(arg);
    self->loop();
	//graceful shutdown：loop 退出後 drain 剩餘事件
    self->drain_epoll();
    return nullptr;
}

//丟給 threadpool 去做的事情
void EpollWorker::enqueue_event_wrapper(void* arg) {
    std::unique_ptr<Event> e(static_cast<Event*>(arg));
	// 回傳訊息給 client
	std::cout << "Server feedback msg! " << std::endl;
    std::string msg = "Hello EpollWorker to Client!";
    ssize_t n = write(e->fd, msg.c_str(), msg.size());
	//1.(查詢部部分)這裡如果有需要傳回的，但已經斷線的就不用傳回了
	//沒有send 還是可以做資料處理
    if (n < 0) {
        perror("write failed");
    }
    std::cout << "Event on fd " << e->fd
              << " data: " << e->data << std::endl;
}

void EpollWorker::loop() {
    const int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];

    while (!(*stop_all)) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 100);
        if (n < 0) {
            if (errno == EINTR) continue; //防止此thread 被抓去做其他事情
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < n; ++i) {
            handle_event(events[i].data.fd);
        }
    }
}

void EpollWorker::drain_epoll() {
    const int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];
    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 0); // 立即返回
        if (n <= 0) break;
        for (int i = 0; i < n; ++i) {
            handle_event(events[i].data.fd);
        }
    }
}

void EpollWorker::handle_event(int fd) {
    char buf[1024];
    while (true) {
        int nbytes = read(fd, buf, sizeof(buf));
        if (nbytes > 0) {
            try {
                auto e = std::make_unique<Event>(Event{fd, std::string(buf, nbytes)});
                pool.enqueue(&EpollWorker::enqueue_event_wrapper, e.release());
            } catch (const std::exception& ex) {
                std::cerr << "Exception in handle_event: " << ex.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in handle_event" << std::endl;
            }
        } else if (nbytes == 0) { // 對端關閉
            close_fd(fd);
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break; // buffer 已空
			if (errno == EINTR) continue;
            close_fd(fd);
            break;
        }
    }
}

void EpollWorker::close_fd(int fd) {
    close(fd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}
}




