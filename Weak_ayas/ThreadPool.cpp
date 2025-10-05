#include <iostream>
#include <pthread.h>
#include <queue>
#include <vector>
#include <unistd.h> // sleep
#include <atomic>
#include "ThreadPool.hpp"

using namespace std;
namespace threadpool{
void* ThreadPool::workerHelper(void* arg) {
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	pool->worker();
	return nullptr;
}

void ThreadPool::worker() {
	while (true) {
		Task task;

		pthread_mutex_lock(&queue_mutex);
		while (!stop && tasks.empty())
			pthread_cond_wait(&condition, &queue_mutex);//暫時解鎖，並卡在此等待
		
		//當程式結束時 且 沒有任務時，即可結束
		if (stop && tasks.empty()) {
			pthread_mutex_unlock(&queue_mutex);
			break;
		}

		task = tasks.front();
		tasks.pop();
		pthread_mutex_unlock(&queue_mutex);

		try {
			task.func(task.arg); // 執行任務
		} catch (...) {
			std::cerr << "Task exception" << std::endl;
		}
	}
}

ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
	pthread_mutex_init(&queue_mutex, nullptr);
	pthread_cond_init(&condition, nullptr);

	for (size_t i = 0; i < numThreads; ++i) {
		pthread_t tid;
		if (pthread_create(&tid, nullptr, workerHelper, this) != 0) {
			perror("pthread_create failed");
		}
		workers.push_back(tid);
	}
}

ThreadPool::~ThreadPool() {
	pthread_mutex_lock(&queue_mutex);
	stop = true;
	pthread_mutex_unlock(&queue_mutex);
	
	//通知所有thread，相當於condition.notify_all();
	pthread_cond_broadcast(&condition);

	for (pthread_t tid : workers)
		pthread_join(tid, nullptr);

	pthread_mutex_destroy(&queue_mutex);
	pthread_cond_destroy(&condition);
	std::cout << "ThreadPool is stoped!" << std::endl;
}

void ThreadPool::enqueue(void (*func)(void*), void* arg) {
	pthread_mutex_lock(&queue_mutex);
	tasks.push(Task{func, arg});
	pthread_mutex_unlock(&queue_mutex);
	
	//通知任一 thread，相當於condition.notify_one();
	pthread_cond_signal(&condition);
}
}




