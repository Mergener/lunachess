#include <lunachess.h>

#include <string>
#include <vector>
#include <exception>

#include "lunatest.h"

namespace lunachess::tests {

extern std::unordered_map<std::string, std::vector<TestCase>> testGroups;
void createTests();

class AssertionFailure {
public:
    std::string testGroup;
    i32 caseIndex;
    std::string assertMessage;
    std::string assertFunction;
    std::string assertFile;
    i32 assertLine;

    AssertionFailure(std::string testGroup,
                     i32 caseIndex,
                     std::string assertMessage,
                     std::string assertFunction,
                     std::string assertFile,
                     i32 assertLine)
     : testGroup(std::move(testGroup)),
       caseIndex(caseIndex),
       assertMessage(std::move(assertMessage)),
       assertFunction(std::move(assertFunction)),
       assertFile(std::move(assertFile)),
       assertLine(assertLine) {}
};

struct TestContext {
    i32 nTests = 0;
    i32 nTestsPassed = 0;

    std::string currentTestName;
    i32 currentTestCase;
    std::vector<AssertionFailure> assertionFailures;
};

i32 testMain() {
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

    // We store assertion failures at the context to log them only
    // after running all tests.
    debug::setAssertFailHandler([&ctx](const char* fileName, const char* funcName, i32 line, std::string_view msg) {
        ctx.assertionFailures.emplace_back(ctx.currentTestName, ctx.currentTestCase, std::string(msg), funcName, fileName, line);
    });

    std::cout << "The following test groups will be executed: " << std::endl;
    for (const auto& pair: testGroups) {
        std::cout << "\t- " << pair.first << std::endl;
    }

    std::cout << "\nRunning tests..." << std::endl;
    std::cout << std::endl;
    i32 i = 0; // Used to count how many tests have been executed so far
    for (const auto& pair: testGroups) {
        TERMINAL_ERASE_LINE();
        std::cout << i << " of " << testGroups.size() << " tests finished...";

        auto name = pair.first;
        const auto& group = pair.second;
        try {
            ctx.nTests++;
            ctx.currentTestName = name;
            std::cout << " (running test group '" << ctx.currentTestName << "')";
            std::cout.flush();

            for (i32 j = 0; j < group.size(); ++j) {
                ctx.currentTestCase = j;
                group[j]();
            }

            ctx.nTestsPassed++;
        }
        catch (const std::exception& e) {
            // Unhandled exceptions are worse than failed assertions in this case, since
            // they can impact the state of further tests or even worse.
            // Log and abort tests.
            TERMINAL_COLOR_ERROR();
            std::cerr << "Unhandled exception occurred during test group " << ctx.currentTestName << ":\n" << e.what() << std::endl;
            TERMINAL_COLOR_DEFAULT();
            throw e;
        }
        i++;
    }

    TERMINAL_ERASE_LINE();
    std::cout << "All tests finished.\n" << std::endl;

    if (ctx.assertionFailures.empty()) {
        // All tests passed :)
        TERMINAL_COLOR_SUCCESS();
        std::cout << "All " << ctx.nTests << " test groups passed." << std::endl;
        TERMINAL_COLOR_DEFAULT();

        return 0;
    }
    // Not all tests passed :(
    TERMINAL_COLOR_ASSERTFAIL();
    i32 nTestFails = ctx.nTests - ctx.nTestsPassed;
    std::cout << nTestFails << " of " << ctx.nTests << " tests passed. " << std::endl;
    TERMINAL_COLOR_DEFAULT();

    std::cout << "\nAssertion failures:" << std::endl;

    std::string currTest = "";
    for (const AssertionFailure& f: ctx.assertionFailures) {
        if (f.testGroup != currTest) {
            // Assertion failures from a single group are placed contiguously.
            // If the test group changes, this means that we are now reporting
            // failures from a different group -- log it.
            currTest = f.testGroup;
            std::cout << "\nTest '" << currTest << "':" << std::endl;
        }

        TERMINAL_COLOR_ASSERTFAIL();
        std::cout << "\tFile: " << f.assertFile << std::endl;
        std::cout << "\tFunction: " << f.assertFunction << std::endl;
        std::cout << "\tLine: " << f.assertLine << std::endl;
        std::cout << "\tTest Case: " << f.caseIndex << std::endl;
        std::cout << "\tMessage: " << f.assertMessage << std::endl;

        std::cout << std::endl;
        TERMINAL_COLOR_DEFAULT();
    }

    // Return the number of failed tests.
    // This should be the return code for our program.
    return nTestFails;
}

} // lunachess::tests

int main() {
    try {
        lunachess::initializeEverything();

        // Setup terminal
        std::ios_base::sync_with_stdio(false);

        TERMINAL_COLOR_DEFAULT();
        std::cout << "Running lunatest with Luna " << LUNA_VERSION_NAME << "."
                  << std::endl;

        i32 ret = lunachess::tests::testMain();

        TERMINAL_COLOR_RESET();

        return ret;
    }
    catch (const std::exception& e) {
        TERMINAL_COLOR_RESET();
        throw e;
    }

}