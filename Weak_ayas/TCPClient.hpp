#pragma once

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <string>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>

namespace tcpclient {

class ClientTCP {

    std::string server_ip;
    int server_port;
    int sock{-1};
    int epoll_fd{-1};
    int notify_fd{-1};
    std::thread recv_thread;
    std::atomic<bool> running{false};
    std::atomic<bool>* stop_all;
	std::atomic<bool> server_on{false};
	std::atomic<bool> close_all{false};
	
	void set_nonblocking(int fd);
    void recv_loop();
	size_t get_start_payload(const std::string& data);
public:

    ClientTCP(const std::string& ip, int port, std::atomic<bool>* stopAllPtr = nullptr);
    ~ClientTCP();
    bool start();
    void stop();
    std::string get_server_ip() const;
    int get_server_port() const;
    void send_msg(const std::string& msg);
	std::string get_ip() const; // 取得 Client IP
    int get_port() const;       // 取得 Client port
	bool client_get_server_stats();
};

} // namespace tcpclient
