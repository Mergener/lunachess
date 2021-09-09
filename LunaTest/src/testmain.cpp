#include "tests.h"

#include "lunachess.h"
#include "core/debug.h"

#include <iostream>

#define COL_RESET       (!s_ColorsSupported ? "" : "\033[0m")
#define COL_BLACK       (!s_ColorsSupported ? "" : "\033[30m")   
#define COL_RED         (!s_ColorsSupported ? "" : "\033[31m")   
#define COL_GREEN       (!s_ColorsSupported ? "" : "\033[32m")   
#define COL_YELLOW      (!s_ColorsSupported ? "" : "\033[33m")   
#define COL_BLUE        (!s_ColorsSupported ? "" : "\033[34m")   
#define COL_MAGENTA     (!s_ColorsSupported ? "" : "\033[35m")   
#define COL_CYAN        (!s_ColorsSupported ? "" : "\033[36m")   
#define COL_WHITE       (!s_ColorsSupported ? "" : "\033[37m")   
#define COL_BOLDBLACK   (!s_ColorsSupported ? "" : "\033[1m\033[30m")      
#define COL_BOLDRED     (!s_ColorsSupported ? "" : "\033[1m\033[31m")     
#define COL_BOLDGREEN   (!s_ColorsSupported ? "" : "\033[1m\033[32m")     
#define COL_BOLDYELLOW  (!s_ColorsSupported ? "" : "\033[1m\033[33m")     
#define COL_BOLDBLUE    (!s_ColorsSupported ? "" : "\033[1m\033[34m")     
#define COL_BOLDMAGENTA (!s_ColorsSupported ? "" : "\033[1m\033[35m")     
#define COL_BOLDCYAN    (!s_ColorsSupported ? "" : "\033[1m\033[36m")      
#define COL_BOLDWHITE   (!s_ColorsSupported ? "" : "\033[1m\033[37m")      

#define COL_MSG  COL_BOLDCYAN
#define COL_ERR  COL_BOLDRED
#define COL_GOOD COL_BOLDGREEN

namespace lunachess::tests {

static bool s_ColorsSupported = false;

struct {

	const Test* test = nullptr;
	bool passed = false;

} s_CurrentTestData;

static int s_TotalPassed = 0;

static void onAssertFail(const char* fileName,
	                     const char* funcName,
	                     int line,
	                     std::string_view msg) {
	std::cout << COL_ERR;
	std::cout << "\n[Assertion Failure] In file '" << fileName << "', function '" << funcName << "', line " << line << " -- Message:\n";
	std::cout << msg << std::endl;
	std::cout << COL_RESET;

	s_CurrentTestData.passed = false;
}

static void doTest(int testIdx) {
	s_CurrentTestData.test = &g_Tests[testIdx];
	s_CurrentTestData.passed = true;

	std::cout << COL_MSG << "\n** Starting '" << s_CurrentTestData.test->name << "'...\n" << COL_RESET;

	s_CurrentTestData.test->function();

	if (s_CurrentTestData.passed) {
		s_TotalPassed++;
		std::cout << COL_GOOD << "\n** '" << s_CurrentTestData.test->name << "' passed.\n";
	}
	else {
		std::cout << COL_ERR << "\n** '" << s_CurrentTestData.test->name << "' failed.\n";
	}

	std::cout << COL_RESET;
}

static void testAll() {
	for (int i = 0; i < g_TestCount; ++i) {
		doTest(i);
	}
}

}

int main() {
	using namespace lunachess::tests;

#ifndef LUNA_ASSERTS_ON
	std::cerr << "Recompile Lunatest with 'LUNA_ASSERTS_ON' defined! Exiting.\n";
	return EXIT_FAILURE;
#endif

	std::ios_base::sync_with_stdio(false);

	lunachess::initialize();
	lunachess::debug::setAssertFailHandler(onAssertFail);

	testAll();
	std::cout << COL_MSG << "\nTesting finished. (" << s_TotalPassed << " of " << g_TestCount << " passed)" << COL_RESET << std::endl;

	std::cin.get();
	return 0;
}