//
// Created by Thomas Mergener on 04/10/2021.
//

#include "strutils.h"

#include <algorithm>

namespace lunachess::strutils {

void reduceWhitespace(std::string& s) {
	std::string::iterator newEnd = std::unique(s.begin(), s.end(), [](char a, char b) {
		return (a == b) && (a == ' ');
	});
	s.erase(newEnd, s.end());
}

void split(std::string_view s, std::vector<std::string_view>& tokens, std::string_view delim) {
	size_t last = 0;
	size_t next;
	while ((next = s.find(delim, last)) != std::string::npos) {
		tokens.push_back(s.substr(last, next - last));
		last = next + 1;
	}
	tokens.push_back(s.substr(last));
}

}