#include "TCPServer.hpp"
#include "ThreadPool.hpp"
#include "EpollWorker.hpp"
#include "TCPClient.hpp"
#include "protocol.hpp"
#include "SQLManager.hpp"
#include "ConsoleClient.hpp"

#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <csignal>
#include <unordered_map>
#include <condition_variable>
#include <mutex>

//宏
//#define DEBUG_MODEf
//#define RELEASE
using namespace threadpool;
using namespace epollworker;
using namespace tcpserver;
using namespace tcpclient; // ClientTCP namespace
using namespace sqlmanager;
using namespace protocol;
using namespace protocol::sql_protocol;
using namespace console_client;
using namespace std;

#define SERVER_PORT 5500
#define THREAD_POOL_SIZE 4
#define CLIENTS 5

atomic<bool> g_stop_flag(false);
std::atomic<bool> g_stop_client_flag(false);
mutex mtx;
condition_variable cv;

void signal_handler(int signum) {
    g_stop_flag = true;
    cv.notify_one(); // 喚醒等待中的主執行緒
}

void Server_loop(ThreadPool& pool, bool& server_running){
    while(!g_stop_flag.load()){

        TCPServer server(SERVER_PORT, pool, 2, &g_stop_flag, mtx, cv); //兩個 EpollWorker

        if (!server.start()) {
            cerr << "Server start failed!" << endl;
            {
                std::lock_guard<std::mutex> lock(mtx);
                server_running = false;    // 明確告訴主 thread server 沒啟動成功
                cv.notify_all();           // 喚醒等待的主 thread
            }
            return;
        }

        cout << "Server started on port: " << SERVER_PORT << endl;
        server_running = true;
        {
            unique_lock<mutex> lock(mtx);
            cv.notify_one(); // 確保 client thread 不會錯過通知
            cv.wait(lock, [&server] {
                return !server.get_serverstats() || g_stop_flag.load();
            });
        }

        cout << "Stopping server..." << endl;
        //server.stop(); 迴圈結束，會自動呼叫析構

        if (g_stop_flag.load()) {
            cout << "Exiting due to signal." << endl;
            break;
        } else {
            cout << "Restarting server..." << endl;
        }
    }
}

int main() {

    signal(SIGINT, signal_handler); //中斷程式
    bool server_running = false;

    ThreadPool pool(4); // 共用 threadpool
    SQLManager mgr_SQL;
    thread server_thread([&pool,&server_running](){
        Server_loop(pool,server_running);
    }); //啟動 server 迴圈
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&server_running]{ return server_running; }); // 等待 server 啟動
    }

    #if defined(MULTI_CONNECT)
    // 啟動多個 ClientTCP
    std::vector<std::unique_ptr<ClientTCP>> clients;
    std::vector<std::thread> client_threads;
    vector<atomic<bool>> client_running(CLIENTS);
    std::unordered_map<int, string> client_names; // 儲存 client 的名稱
    client_names[0] = "Mallory";
    client_names[1] = "Alice";
    client_names[2] = "Bob";
    client_names[3] = "Eve";
    client_names[4] = "Abby";
    string specialname = "Oscar"; // 特殊用戶名稱

    //加入5個 client
    for (int i = 0; i < CLIENTS; ++i) {
        clients.emplace_back(
            std::make_unique<ClientTCP>("127.0.0.1", SERVER_PORT, &g_stop_client_flag)
        );
    }
    //std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 等待 server 啟動

    // 啟動 client thread，定時發訊息
    for (int i = 0; i < CLIENTS; ++i) {

        client_threads.emplace_back([i,&client_running, &mgr_SQL, &client_names, &clients]() {

            ClientTCP* client = clients[i].get();

            if (!client->start()) {
                std::cerr << "Client " << i << " failed to start" << std::endl;
                return;
            }
            string name = client_names[i];
            std::string msg = "";
            // 登入測試錯誤
            msg = "#" + name;
            msg = mgr_SQL.build_package(LOGIN, msg);
            client->send_msg(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // 註冊用戶測試
            msg = "#" + name;
            msg = mgr_SQL.build_package(REGISTER_USER, msg);
            client->send_msg(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // 登入測試
            msg = "#" + name;
            msg = mgr_SQL.build_package(LOGIN, msg);
            client->send_msg(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // 查找所有單元的解題成功率測試
            msg = ""; // 無參數
            msg = mgr_SQL.build_package(SELECT_UNITS_DATA, msg);
            client->send_msg(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // 查找各別單元的解題情況測試
            msg = "#Array"; // 假設查詢單元1
            msg = mgr_SQL.build_package(SELECT_UNIT_DETAIL, msg);
            client->send_msg(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            // 插入提交狀況測試
            msg = "#" + to_string(1) + "#" + "true" + "#" + "00:10:30"; // 假設題號001，解題成功，解題時間 00:10:30
            msg = mgr_SQL.build_package(INSERT_SUBMITTED, msg);
            client->send_msg(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // 查詢某題的提交狀況測試
            msg = "#" + to_string(1); // 假設題號1001
            msg = mgr_SQL.build_package(GET_PROBLEM_SUBMISSIONS, msg);
            client->send_msg(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // 刪除提交狀況測試
            /*std::string msg = ; // 假設刪除 user1 的 problem1001 提交紀錄
            msg = mgr_SQL.build_package(DELETE_SUBMITTED, msg);
            client->send_msg(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));*/

            client_running[i] = true;
            cv.notify_all(); // 通知主執行緒該 client 已完成跑完
            client->stop();
        });
    }

    {
        unique_lock<mutex> lock(mtx);
        // 等待測試時間 + 信號中斷
        cv.wait(lock, [&client_running]{ 
            bool alldone = true;
            for(int i = 0;i < CLIENTS && alldone;i++){
                alldone = alldone && client_running[i]; 
            }
            return alldone;
        }); // 等待 server 啟動或中斷信號
    }


    std::cout << "Client Allstop!" << std::endl;
    g_stop_client_flag = true;
    for (auto &t : client_threads) t.join();
    #endif

    #if defined(AUTO_SINGLE_CONNECT)
    // 建立模擬輸入
    std::istringstream fake_input(
        "help\n"

        "login\n"
        "Oscar\n"

        "register\n"
        "123456\n"

        "delete_user\n"
        "123456\n"

        "insert_problem\n"
        "11\n"
        "Container With Most Water\n"
        "Med.\n"

        "insert_type\n"
        "11\n"
        "Two Pointers\n"  

        "insert_submission\n"
        "11\n"
        "1\n"
        "00:10:23\n"

        "get_submission\n"
        "11\n"

        "query_units_data\n"

        "query_unit_detail\n"
        "Two Pointers\n"

        "delete_type\n"
        "11\n"
        "Two Pointers\n"  

        "delete_problem\n"
        "11\n"

        "exit\n"
    );
    //與 fake_input 暫時交換 buffer
    std::streambuf* orig_buf = std::cin.rdbuf(fake_input.rdbuf());
    
    // 建立 ClientTCP
    std::unique_ptr<ClientTCP> client = std::make_unique<ClientTCP>("127.0.0.1", SERVER_PORT, &g_stop_client_flag);
    client->start();
    // 建立 ConsoleClient 並執行
    ConsoleClient console(*client);

    console.run();
    
    // 恢復原本 cin
    std::cin.rdbuf(orig_buf);
    g_stop_client_flag.store(true);
    client->stop();
    #endif

    std::cout << "Set Allstop!" << std::endl;
    g_stop_flag = true;
    cv.notify_all(); // 喚醒 server 迴圈中的等待

    std::cout << "Server stopped!" << std::endl;
    server_thread.join();

    std::cout << "Test finished." << std::endl;
    return 0;
}
