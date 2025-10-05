#include <iostream>
#include <pthread.h>
#include <queue>
#include <vector>
#include <unistd.h> // sleep
#include <atomic>
#include "ThreadPool.hpp"

using namespace std;
using namespace threadpool;
// 封裝任務參數
struct TaskArg {
    int id;
    std::atomic<int>* counter;
};

void testTask0(void* arg) {
    int id = *reinterpret_cast<int*>(arg);
    std::cout << "Task " << id << " is running" << std::endl;
    sleep(1); // 模擬耗時工作
}

void exceptionTask(void* arg) {
    throw std::runtime_error("oops");
}

// 任務函數
void testTask(void* arg) {
    TaskArg* t = static_cast<TaskArg*>(arg);
    try {
        std::cout << "Task " << t->id << " start" << std::endl;
        usleep(100000); // 模擬耗時
        std::cout << "Task " << t->id << " done" << std::endl;
        t->counter->fetch_add(1, std::memory_order_relaxed);
    } catch (...) {
        std::cerr << "Task exception" << std::endl;
    }
}

int main() {
	{
        ThreadPool pool(2);
        int arg = 42;
        pool.enqueue(testTask0, &arg);
		sleep(1);
		std::cout << "scope: " << " tasks completed.\n" << std::endl;
        // 退出 scope 後 pool 會自動 stop 並 join
    } // pool 作用域結束
	
	
    const int numWorkers = 8;
    const int numTasks = 100;

    ThreadPool pool(numWorkers);
	
	//捕獲內部情況
	pool.enqueue(exceptionTask, nullptr);
	
	int args2[5] = {1, 2, 3, 4, 5};
    for(int i = 0; i < 5; ++i)
        pool.enqueue(testTask0, &args2[i]);
	sleep(1);
	std::cout << "test2: 5 tasks completed." << std::endl;

    // 建立任務參數
	std::atomic<int> completedTasks(0);
    std::vector<TaskArg> args(numTasks);
    for (int i = 0; i < numTasks; ++i) {
        args[i].id = i + 1;
        args[i].counter = &completedTasks;
        pool.enqueue(testTask, &args[i]);
    }

    // 等待所有任務完成
    while (completedTasks.load(std::memory_order_relaxed) < numTasks) {
        usleep(50000); // 50ms 等待
    }
	std::cout << "test3: " << numTasks << " tasks completed." << std::endl;

    std::cout << "All " << numTasks << " tasks completed." << std::endl;
	
	
	
    return 0;
}




