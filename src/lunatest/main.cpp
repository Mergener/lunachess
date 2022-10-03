#include <lunachess.h>

#include <string>
#include <vector>
#include <exception>

#include "lunatest.h"

namespace lunachess::tests {

void createTests();
extern std::vector<TestSuite*> g_TestSuites;

class AssertionFailure {
public:
    std::string testName;
    std::string assertMessage;
    std::string assertFunction;
    std::string assertFile;
    int assertLine;

    AssertionFailure(std::string testName,
                     std::string assertMessage,
                     std::string assertFunction,
                     std::string assertFile,
                     int assertLine)
     : testName(std::move(testName)),
       assertMessage(std::move(assertMessage)),
       assertFunction(std::move(assertFunction)),
       assertFile(std::move(assertFile)),
       assertLine(assertLine) {}
};

struct TestContext {
    int nTests = 0;
    int nTestsPassed = 0;

    std::string currentTestName;
    std::vector<AssertionFailure> assertionFailures;
};

int testMain() {
    TERMINAL_COLOR_DEFAULT();
    createTests();

    if (!debug::assertsEnabledInLib()) {
        // LUNA_ASSERT will be working on unit tests, but not
        // inside the functions called by them.
        TERMINAL_COLOR_WARNING();
        std::cout << "WARNING: lunalib has assertions disabled." << std::endl;
        std::cout << "No assertions will be triggered on the lib, only on the tests themselves." << std::endl;
        std::cout << "Make sure to compile lunalib with LUNA_ASSERTS_ON." << std::endl;
        std::cout << std::endl;
        TERMINAL_COLOR_DEFAULT();
    }

    TestContext ctx;

    debug::setAssertFailHandler([&ctx](const char* fileName, const char* funcName, int line, std::string_view msg) {
        ctx.assertionFailures.emplace_back(ctx.currentTestName, std::string(msg), funcName, fileName, line);
    });

    std::cout << "The following tests will be executed: " << std::endl;
    for (const auto& test: g_TestSuites) {
        std::cout << "\t- " << test->getName() << std::endl;
    }

    std::cout << "\nRunning tests..." << std::endl;
    std::cout << std::endl;
    for (int i = 0; i < g_TestSuites.size(); ++i) {
        TERMINAL_ERASE_LINE();
        std::cout << i << " of " << g_TestSuites.size() << " tests finished...";
        const auto& test = g_TestSuites[i];
        try {
            ctx.nTests++;
            ctx.currentTestName = test->getName();
            std::cout << " (running test '" << ctx.currentTestName << "')";
            std::cout.flush();
            test->run();
            ctx.nTestsPassed++;
        }
        catch (const std::exception& e) {
            TERMINAL_COLOR_ERROR();
            std::cerr << "Unhandled exception occurred during test " << ctx.currentTestName << ":\n" << e.what() << std::endl;
            TERMINAL_COLOR_DEFAULT();
            throw e;
        }
    }

    TERMINAL_ERASE_LINE();
    std::cout << "All tests finished.\n" << std::endl;

    if (ctx.assertionFailures.empty()) {
        TERMINAL_COLOR_SUCCESS();
        std::cout << "All " << ctx.nTests << " tests passed." << std::endl;
        TERMINAL_COLOR_DEFAULT();

        return 0;
    }

    TERMINAL_COLOR_ASSERTFAIL();
    int nTestFails = ctx.nTests - ctx.nTestsPassed;
    std::cout << nTestFails << " of " << ctx.nTests << " tests passed. " << std::endl;
    TERMINAL_COLOR_DEFAULT();

    std::cout << "\nAssertion failures:" << std::endl;

    std::string currTest = "";
    for (const AssertionFailure& f: ctx.assertionFailures) {
        if (f.testName != currTest) {
            currTest = f.testName;
            std::cout << "\nTest '" << currTest << "':" << std::endl;
        }

        TERMINAL_COLOR_ASSERTFAIL();
        std::cout << "\tFile: " << f.assertFile << std::endl;
        std::cout << "\tFunction: " << f.assertFunction << std::endl;
        std::cout << "\tLine: " << f.assertLine << std::endl;
        std::cout << "\tMessage: " << f.assertMessage << std::endl;

        std::cout << std::endl;
        TERMINAL_COLOR_DEFAULT();
    }

    return ctx.nTests - ctx.nTestsPassed;
}

} // lunachess::tests

int main() {
    try {
        lunachess::initializeEverything();

        // Setup terminal
        std::ios_base::sync_with_stdio(false);

        TERMINAL_COLOR_DEFAULT();
        std::cout << "Running lunatest with Luna " << LUNA_VERSION_MAJOR << "."
                  << LUNA_VERSION_MINOR << "."
                  << LUNA_VERSION_PATCH << ".\n"
                  << std::endl;


        int ret = lunachess::tests::testMain();

        TERMINAL_COLOR_RESET();

        return ret;
    }
    catch (const std::exception& e) {
        TERMINAL_COLOR_RESET();
        throw e;
    }

}