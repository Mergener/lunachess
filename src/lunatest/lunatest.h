#ifndef LUNA_TEST_H
#define LUNA_TEST_H

#include <string>

#include "rang.h"

#define TERMINAL_ERASE_LINE()       (std::cout << "\33[2K\r")
#define TERMINAL_COLOR_DEFAULT()    (std::cout << rang::bg::reset << rang::style::reset << rang::fgB::cyan)
#define TERMINAL_COLOR_WARNING()    (std::cout << rang::fgB::yellow)
#define TERMINAL_COLOR_ERROR()      (std::cout << rang::fgB::red << rang::style::bold << rang::style::underline)
#define TERMINAL_COLOR_SUCCESS()    (std::cout << rang::fgB::green)
#define TERMINAL_COLOR_ASSERTFAIL() (std::cout << rang::fgB::red)
#define TERMINAL_COLOR_RESET()      (std::cout << rang::fg::reset << rang::bg::reset << rang::style::reset)

namespace lunachess::tests {

/**
 * Class for test suites.
 */
class TestSuite {
public:
    virtual void run() = 0;

    inline const std::string& getName() const {
        return m_Name;
    }

protected:
    explicit TestSuite(std::string name)
            : m_Name(std::move(name)) {}

private:
    std::string m_Name;
};

} // lunachess::tests

#endif // LUNA_TEST_H