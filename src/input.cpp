#include "input.h"

#include <thread>
#include <queue>
#include <iostream>

#include "lock.h"

namespace lunachess::input {

static Lock s_Lock;
static bool s_ShouldDie = false;
static std::thread* s_Thread = nullptr;
static std::queue<std::string> s_InputQueue;

static void inputThreadMain() {
    while (!s_ShouldDie) {
        std::string input;
        std::getline(std::cin, input);

        s_Lock.lock();
        s_InputQueue.push(input);
        s_Lock.unlock();
    }
}

void initializeThread() {
    s_Thread = new std::thread(inputThreadMain);
}

void killThread() {
    s_ShouldDie = true;
}

bool poll(std::string& out) {
    if (s_InputQueue.size() == 0) {
        return false;
    }

    s_Lock.lock();

    out = s_InputQueue.front();
    s_InputQueue.pop();

    s_Lock.unlock();
    return true;
}

}