#ifndef LUNA_STRUTILS_H
#define LUNA_STRUTILS_H

#include <string>
#include <string_view>
#include <vector>

namespace lunachess::strutils {

void reduceWhitespace(std::string& s);
void split(std::string_view s, std::vector<std::string_view>& tokens, std::string_view delim);

}

#endif //LUNA_STRUTILS_H
