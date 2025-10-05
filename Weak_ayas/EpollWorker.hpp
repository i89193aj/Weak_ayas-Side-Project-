#pragma once

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <atomic>
#include <string>
#include <memory>
#include <cstdint>
#include <array>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <cstring>  // 必須包含

#include "ThreadPool.hpp"
#include "SQLManager.hpp"

namespace threadpool {
    class ThreadPool;
}

namespace sqlmanager{
	class SQLManager;
}

namespace protocol{
	class SocketPackage;
	//namespace sql_protocol{} 沒用到不需要寫，cpp那邊在寫!
}

namespace epollworker {
// Epoll 事件
struct Event {
    int fd;
    std::string data;
};

// 每個 fd 的管理資訊
struct fd_info {
    std::shared_ptr<std::mutex> fd_mutex;
    std::thread fd_thread;
    std::condition_variable fd_cond;
    bool fd_inthreadpool;
    std::atomic<bool> needactive;
    std::atomic<bool> thread_running;

    fd_info() 
        : fd_mutex(std::make_shared<std::mutex>()), 
          fd_inthreadpool(false), 
          needactive(false), 
          thread_running(false) {}
};

class EpollWorker {
private:
    int epoll_fd;
    pthread_t worker_thread;
    threadpool::ThreadPool &pool;
	std::atomic<bool> running{false};
    std::atomic<bool>* stop_all;

    std::unordered_map<int, std::shared_ptr<fd_info>> fd_infos;
    std::mutex fd_map;
	
	sqlmanager::SQLManager mgr_SQL;

    static void* threadFunc(void* arg);
    void set_nonblocking(int fd) const;

    static void enqueue_event_wrapper(void* arg); // static, 用 struct 傳 this + Event*
    void enqueue_event_wrapper_nonstatic(Event* e);
	void active_enqueue_event(Event* e);

    void loop();
    void drain_epoll();
    void handle_event(int fd);
    bool activefd_handle_event(int fd);
    void close_fd(int fd);

    bool get_lock_fdinfo(std::shared_ptr<fd_info>& info_ptr, int fd); //找到對應fd資訊 (裡面會鎖住)
	bool parse_sockpackage(const std::string& data, protocol::SocketPackage& pkg);

public:
    explicit EpollWorker(threadpool::ThreadPool& pool, std::atomic<bool>* stop_all_ptr);
    ~EpollWorker();

    // 禁止 copy/move
    EpollWorker(const EpollWorker&) = delete;
    EpollWorker& operator=(const EpollWorker&) = delete;
    EpollWorker(EpollWorker&&) = delete;
    EpollWorker& operator=(EpollWorker&&) = delete;

    void add_fd(int fd);
};

struct EventWrapper {
    EpollWorker* self;
    Event* e;
};

} // namespace epollworker
