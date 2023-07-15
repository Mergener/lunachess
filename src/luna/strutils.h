#ifndef LUNA_STRUTILS_H
#define LUNA_STRUTILS_H

#include <string>
#include <string_view>
#include <vector>
#include <type_traits>
#include <cctype>

#include "types.h"

namespace lunachess::strutils {

/**
 * Removes redundant whitespace in-place for a given string.
 * Ex:
 * "The          quick         brown    fox jumps    over  the lazy     dog."
 * becomes
 * "The quick brown fox jumps over the lazy dog."
 */
void reduceWhitespace(std::string& s);
void split(std::string_view s, std::vector<std::string_view>& tokens, std::string_view delim);

void toLower(std::string& s);
void toUpper(std::string& s);

template <typename T>
std::string toString(const T& x) {
    std::stringstream ss;
    ss << x;
    return ss.str();
}

template <typename TInt>
bool tryParseInteger(std::string_view sv, TInt& ret) {
    static_assert(std::is_integral<TInt>::value);

    if (sv.size() <= 0) {
        // No characters
        return false;
    }

    ret = 0;
    TInt mult = 1;
    int i = 0;

    if (sv[0] == '-') {
        mult = -1;
        i++;
    }

    while (std::isdigit(sv[i])) {
        ret *= 10;
        ret += sv[i] - '0';
        i++;
    }

    ret *= mult;

    return true;
}

template <typename TInt>
TInt parseInteger(std::string_view sv) {
    static_assert(std::is_integral<TInt>::value);

    TInt ret;

    if (!tryParseInteger(sv, ret)) {
        throw std::runtime_error("Invalid integer");
    }

    return ret;
}

} // lunachess

#endif //LUNA_STRUTILS_H
