#include "debug.h"

#include <stdexcept>
#include <iostream>

namespace lunachess::debug {

static bool s_Initialized = false;

class AssertionFailureException : public std::runtime_error {
public:
    AssertionFailureException()
            : std::runtime_error("Assertion failure") { }
};

static void defaultAssertionFailHandler(const char* fileName, const char* funcName, int line, std::string_view msg) {
    std::cerr << "[Assertion Failure] In file " << fileName << ", function " << funcName << ", line " << line << " -- Message:\n";
    std::cerr << msg;
    std::cerr << "\n\n==================";
    std::cin.get();
    throw AssertionFailureException();
}

static AssertionFailHandler s_AssertionFailHandler = defaultAssertionFailHandler;

void assertionFailure(const char* fileName, const char* funcName, int line, std::string_view msg) {
    s_AssertionFailHandler(fileName, funcName, line, msg);
}

void setAssertFailHandler(AssertionFailHandler h) {
    if (h != nullptr) {
        s_AssertionFailHandler = h;
    }
    else {
        s_AssertionFailHandler = defaultAssertionFailHandler;
    }
}

}