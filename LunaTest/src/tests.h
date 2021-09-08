#ifndef LUNA_TESTS_H
#define LUNA_TESTS_H

#include <cstdlib>
#include <string>
#include <string_view>
#include <functional>

#include "core/debug.h"

namespace lunachess::tests {

struct Test {
	const std::string name;
	const std::function<void()> function;

	inline Test(std::string_view name, std::function<void()> testFn)
		: name(name), function(testFn) { }
};

extern const Test g_Tests[];
extern const std::size_t g_TestCount;

int testMain();

}

#endif // LUNA_TESTS_H