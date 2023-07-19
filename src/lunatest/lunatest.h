#ifndef LUNA_TEST_H
#define LUNA_TEST_H

#define LUNA_ASSERTS_ON
#include <lunachess.h>

#include <string>
#include <functional>

#include "rang/rang.h"

#define TERMINAL_ERASE_LINE()       (std::cout << "\33[2K\r")
#define TERMINAL_COLOR_DEFAULT()    (std::cout << rang::bg::reset << rang::style::reset << rang::fgB::cyan)
#define TERMINAL_COLOR_WARNING()    (std::cout << rang::fgB::yellow)
#define TERMINAL_COLOR_ERROR()      (std::cout << rang::fgB::red << rang::style::bold << rang::style::underline)
#define TERMINAL_COLOR_SUCCESS()    (std::cout << rang::fgB::green)
#define TERMINAL_COLOR_ASSERTFAIL() (std::cout << rang::fgB::red)
#define TERMINAL_COLOR_RESET()      (std::cout << rang::fg::reset << rang::bg::reset << rang::style::reset)

namespace lunachess::tests {

using TestCase = std::function<void()>;

} // lunachess::tests

#endif // LUNA_TEST_H