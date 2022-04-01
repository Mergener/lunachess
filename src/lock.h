#ifndef LOCK_H
#define LOCK_H

#include <mutex>

namespace lunachess {

class Lock {
public:
    inline void lock() {
        if (m_Destroyed) {
            return;
        }
        m_Lock.lock();
    }

    inline void unlock() {
        m_Lock.unlock();
    }

    ~Lock() {
        m_Destroyed = true;
        m_Lock.unlock();
    }

private:
    std::mutex m_Lock;
    bool m_Destroyed = false;
};

}

#endif //LOCK_H
