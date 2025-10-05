#include "TCPServer.hpp"
#include "ThreadPool.hpp"
#include "EpollWorker.hpp"

#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

using namespace threadpool;
using namespace epollworker;
using namespace tcpserver;

const int TEST_PORT = 12345;
const int CLIENTS = 5;
const int DURATION_SEC = 5;

std::atomic<bool> g_stop_flag(false);

void signal_handler(int signum) {
    g_stop_flag = true;
}

// client thread function
void client_thread_func(int id, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("client socket"); return; }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); 
        close(sock);
        return;
    }

    // 設 non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	
	
    for (int i = 0; i < DURATION_SEC; ++i) {
        std::string msg = "Client " + std::to_string(id) + " msg " + std::to_string(i);
        send(sock, msg.c_str(), msg.size(), 0);

        // drain 回覆
        char buf[1024];
        while (true) {
            ssize_t n = read(sock, buf, sizeof(buf));
            if (n > 0) {
                std::string reply(buf, n);
				std::cout << "Client: " << id <<" ,Recv: "<< reply << std::endl;
            } else if (n == 0) {
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                else if (errno == EINTR) continue;
                else { perror("read"); break; }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // drain 最後可能沒收完的訊息
    char buf[1024];
    while (true) {
        ssize_t n = read(sock, buf, sizeof(buf));
        if (n > 0){
			std::string reply(buf, n);
			std::cout << "(drain)Client: " << id <<" ,Recv: "<< reply << std::endl;
			continue;
		} 
        else if (n == 0) break;
        else if (errno == EAGAIN || errno == EWOULDBLOCK) break;
        else if (errno == EINTR) continue;
        else { perror("read"); break; }
    }

    close(sock);
}

int main() {
    signal(SIGINT, signal_handler);

    ThreadPool pool(4); // 共用 threadpool
    TCPServer server(TEST_PORT, pool, 2, &g_stop_flag);

    if (!server.start()) {
        std::cerr << "Server start failed!" << std::endl;
        return 1;
    }

    std::cout << "Server started on port " << TEST_PORT << std::endl;

    // 啟動多 client thread
    std::vector<std::thread> clients;
    for (int i = 0; i < CLIENTS; ++i) {
        clients.emplace_back(client_thread_func, i, TEST_PORT);
    }

    // 等待測試時間
    for (int i = 0; i < DURATION_SEC + 2 && !g_stop_flag; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
	
	std::cout << "set stop_flag" << std::endl;
    g_stop_flag = true;
    server.stop();
	std::cout << "server is stop!" << std::endl;


    // 等待 client thread 完成
    for (auto &t : clients) t.join();

    std::cout << "Test finished." << std::endl;
    return 0;
}
