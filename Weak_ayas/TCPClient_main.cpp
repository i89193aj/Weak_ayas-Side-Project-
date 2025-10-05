#include "TCPServer.hpp"
#include "ThreadPool.hpp"
#include "EpollWorker.hpp"
#include "TCPClient.hpp"

#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <csignal>
#include <condition_variable>
#include <mutex>

//宏
//#define DEBUG_MODEf
//#define RELEASE
using namespace threadpool;
using namespace epollworker;
using namespace tcpserver;
using namespace tcpclient; // ClientTCP namespace

const int TEST_PORT = 12345;
const int CLIENTS = 5;
const int DURATION_SEC = 5;

std::atomic<bool> g_stop_flag(false);
std::atomic<bool> g_stop_client_flag(false);
std::mutex mtx;
std::condition_variable cv;

void signal_handler(int signum) {
    g_stop_flag = true;
}

int main() {

    signal(SIGINT, signal_handler); //中斷程式

    ThreadPool pool(4); // 共用 threadpool
    TCPServer server(TEST_PORT, pool, 2, &g_stop_flag,mtx,cv); //兩個 EpollWorker

    if (!server.start()) {
        std::cerr << "Server start failed!" << std::endl;
        return 1;
    }

    std::cout << "Server started on port " << TEST_PORT << std::endl;

    // 啟動多個 ClientTCP
    std::vector<std::unique_ptr<ClientTCP>> clients;
    std::vector<std::thread> client_threads;

    for (int i = 0; i < CLIENTS; ++i) {
        clients.emplace_back(std::make_unique<ClientTCP>("127.0.0.1", TEST_PORT, &g_stop_client_flag));
    }

    // 啟動 client thread，定時發訊息
    for (int i = 0; i < CLIENTS; ++i) {

        client_threads.emplace_back([i, &clients]() {

            ClientTCP* client = clients[i].get();

            if (!client->start()) {
                std::cerr << "Client " << i << " failed to start" << std::endl;
                return;
            }
            
			for (int t = 0; t < DURATION_SEC && client->client_get_server_stats(); ++t) {

                std::string msg = "Client: " + std::to_string(i) + " ,msg: " + std::to_string(t);
                client->send_msg(msg);
				std::cout << msg << std::endl;

				
				//!每一秒傳一次訊號
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            client->stop();
        });
    }
	
	#if defined(DEBUG_MODE)
	std::this_thread::sleep_for(std::chrono::milliseconds(2000)); //確認server 有處理完剩下的事情
	#elif defined(RELEASE)
    // 等待測試時間 + 信號中斷
    for (int i = 0; i < DURATION_SEC + 2 && !g_stop_flag; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //等待 DURATION_SEC + 2 秒
    }
	#endif

    std::cout << "Set Allstop!" << std::endl;
    g_stop_flag = true;

    server.stop();
    std::cout << "Server stopped!" << std::endl;

    for (auto &t : client_threads) t.join();

    std::cout << "Test finished." << std::endl;
    return 0;
}
