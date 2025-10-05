#pragma once
#include <iostream>
#include <pthread.h>
#include <queue>
#include <vector>
#include <unistd.h> // sleep
#include <atomic>

namespace threadpool{
struct Task {
    void (*func)(void*); // 任務函式
    void* arg;           // 輸入參數
};

// ThreadPool 類別
class ThreadPool {
	std::vector<pthread_t> workers;
    std::queue<Task> tasks;
    pthread_mutex_t queue_mutex;
    pthread_cond_t condition;
    bool stop;
	static void* workerHelper(void* arg);
    void worker();
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();
    void enqueue(void (*func)(void*), void* arg);
};
}




