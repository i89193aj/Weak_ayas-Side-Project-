#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>

class ThreadPool {
public:
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {   // 取得任務
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty())
                            return; // 結束 thread
                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    try {
                        task(); // 執行任務
                    } catch (const std::exception& e) {
                        std::cerr << "Task exception: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "Task unknown exception" << std::endl;
                    }
                }
            });
        }
    }

    // 提交任務，支援返回值
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> 
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    // 關閉 thread pool
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }

private:
    std::vector<std::thread> workers;            // worker threads
    std::queue<std::function<void()>> tasks;     // 任務佇列

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// 範例使用
int main() {
    ThreadPool pool(4); // 4 個 worker thread

    auto future1 = pool.enqueue([](int a, int b) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return a + b;
    }, 3, 7);

    auto future2 = pool.enqueue([] {
        std::cout << "Hello from thread pool!" << std::endl;
    });

    std::cout << "Result: " << future1.get() << std::endl;
    future2.get(); // 等待任務完成

    return 0;
}


