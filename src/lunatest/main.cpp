#include <lunachess.h>

#include <string>
#include <vector>
#include <exception>

#include "test.h"

namespace lunachess::tests {

extern std::vector<Test> g_Tests;

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

}

int main() {
    using namespace lunachess;
    using namespace lunachess::tests;

    TestContext ctx;

    std::cout << "Running tests...";

    debug::setAssertFailHandler([&ctx](const char* fileName, const char* funcName, int line, std::string_view msg) {
        throw AssertionFailure(ctx.currentTestName, std::string(msg), funcName, fileName, line);
    });

    for (const auto& test: g_Tests) {
        try {
            ctx.nTests++;
            ctx.currentTestName = test.name;
            test.testFunction();
            ctx.nTestsPassed++;
        }
        catch (const AssertionFailure& af) {
            ctx.assertionFailures.push_back(af);
        }
        catch (const std::exception& e) {
            std::cerr << "Exception occurred during test " << ctx.currentTestName << ":\n" << e.what() << std::endl;
        }
    }

    std::cout << ""
}