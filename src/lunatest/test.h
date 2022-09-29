#ifndef LUNA_TESTSUITE_H
#define LUNA_TESTSUITE_H

#include <functional>
#include <string>

namespace lunachess::tests {

struct Test {
    std::string name;
    std::function<void()> testFunction;

    inline Test(std::string name, std::function<void()> testFunction)
        : name(std::move(name)), testFunction(std::move(testFunction)) {}
};

} // lunachess::tests

#endif // LUNA_TESTSUITE_H