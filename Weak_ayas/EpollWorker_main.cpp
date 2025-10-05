// EpollWorker_main.cpp
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "ThreadPool.hpp"
#include "EpollWorker.hpp"

using namespace threadpool;
using namespace epollworker;

const int PORT = 12345;
const int CLIENTS = 2000;
const int DURATION_SEC = 5;

// 建立非阻塞 listen socket
int create_listen_socket(int port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); exit(1); }
    if (listen(listen_fd, 16) < 0) { perror("listen"); exit(1); }

    // 設 non-blocking
    int flags = fcntl(listen_fd, F_GETFL, 0);
    fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK);

    return listen_fd;
}

// client thread function
void client_thread_func(int id) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("client socket"); return; }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); 
        close(sock);
        return;
    }

    // 設 non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    int feedback = 1;

    // 發訊息 loop
    for (int i = 0; i < DURATION_SEC; ++i) {
        std::string msg = "Client " + std::to_string(id) + " msg " + std::to_string(i);
        send(sock, msg.c_str(), msg.size(), 0);

        // 讀取所有可用 server 回覆
        char buf[1024];
        while (true) {
            ssize_t n = read(sock, buf, sizeof(buf));
            if (n > 0) {
                std::string reply(buf, n);
                //std::cout << "Client " << id << " received: " << reply 
                          //<< " feedback: " << feedback++ << std::endl;
            } else if (n == 0) {
                // server 關閉
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else if (errno == EINTR) {
                    continue;
                } else {
                    perror("read");
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // drain 最後可能還沒收到的回覆
    char buf[1024];
    while (true) {
        ssize_t n = read(sock, buf, sizeof(buf));
        if (n > 0) {
            std::string reply(buf, n);
            //std::cout << "Client " << id << std::endl;
        } else if (n == 0) break;
        else if (errno == EAGAIN || errno == EWOULDBLOCK) break;
        else if (errno == EINTR) continue;
        else { perror("read"); break; }
    }

    close(sock);
}

int main() {
    std::atomic<bool> stop_flag(false);
    ThreadPool pool(4);
    EpollWorker worker(pool, &stop_flag);

    int listen_fd = create_listen_socket(PORT);

    // accept loop thread
    std::thread accept_thread([&]() {
        while (!stop_flag) {
            sockaddr_in client_addr{};
            socklen_t len = sizeof(client_addr);
            int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &len);
            if (client_fd >= 0) {
                worker.add_fd(client_fd);
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } else if (errno == EINTR) {
                continue;
            } else {
                perror("accept");
            }
        }
    });

    // 啟動 clients
    std::vector<std::thread> clients;
    for (int i = 0; i < CLIENTS; ++i) {
        clients.emplace_back(client_thread_func, i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(DURATION_SEC + 2));
    stop_flag = true;

    accept_thread.join();
    for (auto &t : clients){
		t.join();
	}
    close(listen_fd);

    std::cout << "Test finished" << std::endl;
    return 0;
}
