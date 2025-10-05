#include "TCPServer.hpp"
#include "ThreadPool.hpp"

#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <fstream>
#include <mutex>
#include <condition_variable>

using namespace threadpool;
using namespace tcpserver;
using namespace std;

#define SERVER_PORT 5500
#define THREAD_POOL_SIZE 4

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
    //注意：(ctrl + c) 只能在Linux 下測試，VScode 的 terminal 也不行!!
    signal(SIGINT, signal_handler);  //中斷程式(ctrl + c)
    signal(SIGTERM, signal_handler); //中斷程式(kill signal)
    ThreadPool pool(THREAD_POOL_SIZE); // 共用 threadpool 正常來說，程式結束，他才會結束

    //注意：不同進程會複製父的所有東西(包含stdin)，所以這個會讓父的stdin 便成非阻塞!
    //thread kb_thread(keyboard_listener_nonblocking); //監聽 按ESC 鍵，退出loop (vscode 可能無法捕捉到)

    while(!g_stop_flag.load()){

        TCPServer server(SERVER_PORT, pool, 2, &g_stop_flag, mtx, cv); //兩個 EpollWorker

        if (!server.start()) {
            cerr << "Server start failed!" << endl;
            return 1;
        }
		
		std::ofstream(READY_FLAG.c_str()).close(); //Server 已經開啟的flag
        cout << "Server started on port: " << SERVER_PORT << endl;
        
        #if defined(D)  //DEBUG_MODE
        cout << "Server started DISCONNECT_TEST: " << SERVER_PORT << endl;
        //測試自動斷線，看那邊會不會重連
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        //g_stop_flag.store(true);
        server.stop();
        while(server.get_serverstats()){
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << "Server g_stop_flag = " << g_stop_flag.load() << std::endl;
        #else
        {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [&server] {
                return !server.get_serverstats() || g_stop_flag.load();
            });			
        }
        #endif
		//server.stop();
        //g_stop_flag.store(false);
        cout << "Stopping server..." << endl;
        //server.stop(); 迴圈結束，會自動呼叫析構

        if (g_stop_flag.load()) {
            cout << "Exiting due to signal." << endl;
            break;
        } else {
            cout << "Restarting server..." << endl;
        }
    }
	// 在 main 結束前 join kb_thread
	//if(kb_thread.joinable()){
	//	kb_thread.join();
	//}
    
    return 0;
}