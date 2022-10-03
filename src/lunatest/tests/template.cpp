#include "../lunatest.h"

namespace lunachess::tests {

struct TestSuiteTemplate : public TestSuite {
    void run() override {
        // Implement your tests here
    }

    TestSuiteTemplate()
        : TestSuite("template") {}
};

}