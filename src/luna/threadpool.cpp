#include "threadpool.h"

namespace lunachess {

ThreadPool::ThreadPool(size_t numThreads) : m_Stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        m_Workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(m_QueueMutex);
                    m_Condition.wait(lock, [this] { return m_Stop || !m_Tasks.empty(); });

                    if (m_Stop && m_Tasks.empty())
                        return;

                    task = std::move(m_Tasks.front());
                    m_Tasks.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_Stop = true;
    }
    m_Condition.notify_all();

    for (std::thread& worker: m_Workers) {
        worker.join();
    }
}

}