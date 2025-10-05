#pragma once

#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <netinet/in.h>
#include <sys/epoll.h>

#include "ThreadPool.hpp"
#include "EpollWorker.hpp"

namespace threadpool {
    class ThreadPool;
}

namespace epollworker {
    class EpollWorker;
}

namespace tcpserver {

class TCPServer {
    int port;                         // Server Port
    bool running;                     // Server 是否正在運行
    int server_sock{-1};              // Socket file descriptor
    int accept_epoll_fd{-1};          // Epoll 監聽 fd (用來監控 server_sock)
    int notify_fd{-1};                // eventfd，用來通知 accept_loop 關閉
    std::thread accept_thread;        // 接收三次握手完成的 client
    threadpool::ThreadPool& pool;     // 共用的 threadpool
    std::vector<std::unique_ptr<epollworker::EpollWorker>> workers; // EpollWorker vector，用 unique_ptr 確保 server 擁有
    size_t next_worker;               // 均勻分配 client 的索引
    std::atomic<bool>* stop_all;      // 外部控制整個程式是否退出的 flag
	std::mutex* extern_mutexptr = nullptr; //外部的鎖
	std::condition_variable* extern_cvptr = nullptr; //外部條件
	std::atomic<bool> close_all{false}; //把全部停掉，避免重複呼叫stop
	
	std::mutex clients_mutex;   		// 鎖控制 client_sockets
	std::vector<int> client_sockets; // 管理client
	
	
	//func
	void set_nonblocking(int fd); // 將 fd 設為非阻塞
    void accept_loop();           // accept loop，處理新 client 連線
	void notify_false();

public:
    TCPServer(int port, threadpool::ThreadPool& sharedPool, size_t nWorkers, std::atomic<bool>* stopAllPtr, std::mutex& extern_mutex, std::condition_variable& extern_cv);
    ~TCPServer();

    bool start();      // 啟動 server
    void stop();       // 停止 server

    std::string get_ip() const; // 取得 server IP
    int get_port() const;       // 取得 server port
	bool get_serverstats();		  // 通知外部是否才在跑
};

} // namespace tcpserver
