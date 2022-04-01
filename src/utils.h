#ifndef LUNA_UTILS_H
#define LUNA_UTILS_H

#include <string>
#include <sstream>

namespace lunachess::utils {

template <typename T>
std::string toString(const T& val) {
    std::stringstream stream;
    stream << val;
    return stream.str();
}



} // lunachess::utils

#endif // LUNA_UTILS_H
