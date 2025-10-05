#include "TCPClient.hpp"
#include "ConsoleClient.hpp"

#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <csignal>
#include <mutex>
#include <condition_variable>
#include <cstdio>      // for std::remove
#include <unistd.h>    // for STDIN_FILENO
#include <fcntl.h>     // for fcntl
#include <termios.h>   // for termios

using namespace tcpclient;
using namespace console_client;
using namespace std;

#define SERVER_PORT 5500
const std::string READY_FLAG = "/tmp/server_ready.flag";// std::string 不能用 constexpr

atomic<bool> g_stop_flag(false);
mutex mtx;
condition_variable cv;

//訊號處理事件
void signal_handler(int signum) {
    g_stop_flag = true;
    cv.notify_one(); // 喚醒等待中的主執行緒
}

// 把終端設成 raw mode（不需要按 Enter 就能讀到鍵盤輸入）
void set_terminal_mode(bool enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO); // 關掉行緩衝和回顯
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

//非阻塞監聽按：ESC
void keyboard_listener_nonblocking() {
    set_terminal_mode(true);

    // 設 stdin 非阻塞
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    while (!g_stop_flag.load()) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n > 0 && c == 27) { // ESC
            g_stop_flag = true;
            cv.notify_one();
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    set_terminal_mode(false);
}


int main(){
    signal(SIGINT, signal_handler);  //中斷程式(ctrl + c)
    signal(SIGTERM, signal_handler); //中斷程式(kill signal)
    //thread kb_thread(keyboard_listener_nonblocking); //監聽 按ESC 鍵，退出loop (vscode 可能無法捕捉到)

    while(!g_stop_flag.load()){

        // 建立 ClientTCP
		std::unique_ptr<ClientTCP> client = std::make_unique<ClientTCP>("127.0.0.1", SERVER_PORT, &g_stop_flag);
		// 建立 ConsoleClient 並執行
		ConsoleClient console(*client);
		//偵測是否已經開啟
		auto start_time = std::chrono::steady_clock::now();
		while(std::remove(READY_FLAG.c_str()) != 0) {

			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start_time).count();
			if (elapsed >= 10000) { // 超過 10 秒就跳出
				std::perror("Delete server_ready.flag fail.");
				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		
		std::cout << "Server is online.\n";

        if (!client->start()) {
            cerr << "Client start failed!" << endl;
            return 1;
        }
		
		cout << "Client started running! " << endl;
        std::cout << "cin good: " << std::cin.good() 
          << ", eof: " << std::cin.eof() 
          << ", fail: " << std::cin.fail() << std::endl;

		console.run(); //裡面會自動偵測，Server是否斷線!
		client->stop();
		
        cout << "Stopping client..." << endl;
        //server.stop(); 迴圈結束，會自動呼叫析構

        if (g_stop_flag.load()) {
            cout << "Exiting due to signal." << endl;
            break;
        } else {
            cout << "Restarting client..." << endl;
        }
    }
	//if(kb_thread.joinable()) 
		//kb_thread.join();
    
    return 0;
}