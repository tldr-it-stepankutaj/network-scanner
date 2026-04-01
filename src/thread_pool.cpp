#include "../include/thread_pool.hpp"

ThreadPool::ThreadPool(const size_t threads) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    condition.wait(lock, [this] {
                        return stop.load() || !tasks.empty();
                    });
                    if (stop.load() && tasks.empty()) return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
        });
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop.load()) return;
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

void ThreadPool::shutdown() {
    stop.store(true);
    condition.notify_all();
}

ThreadPool::~ThreadPool() {
    stop.store(true);
    condition.notify_all();
    for (auto& thread : workers) {
        if (thread.joinable()) thread.join();
    }
}
