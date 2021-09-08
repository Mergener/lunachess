#ifndef LUNA_ERROR_H
#define LUNA_ERROR_H

#include <iostream>
#include <string_view>
#include <cstdlib>

namespace lunachess { 

inline void runtimeAssert(bool condition, std::string_view msg) {
	if (!condition) {
		std::cerr << msg;
		std::exit(EXIT_FAILURE);
	}
}

} 

#endif // LUNA_ERROR_H