#ifndef LUNA_CONSOLE_H
#define LUNA_CONSOLE_H

#include <iostream>

#include "lunachess.h"

namespace lunachess::console {

template <typename T>
void write(const T& val) {
	std::cout << val;
	std::cout.flush();
}

template <typename T>
void writeError(const T& val) {
	std::cerr << val;
	std::cerr.flush();
}

}

#endif // LUNA_CONSOLE_H