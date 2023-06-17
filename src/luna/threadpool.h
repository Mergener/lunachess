#ifndef LUNA_THREADPOOL_H
#define LUNA_THREADPOOL_H

#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace lunachess {

class ThreadPool {
public:
    ThreadPool(size_t numThreads);

    ~ThreadPool();

    template<class F, class... Args>
    void enqueue(F&& f, Args&& ... args) {
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_Tasks.emplace([f, args...] { f(args...); });
        }
        m_Condition.notify_one();
    }

private:
    std::vector<std::thread> m_Workers;
    std::queue<std::function<void()>> m_Tasks;
    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    bool m_Stop;
};

}

#endif  // LUNA_THREADPOOL_H