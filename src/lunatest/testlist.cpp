#include "lunatest.h"

#include <vector>

#define LUNA_ASSERTS_ON
#include "tests/perfttests.cpp"

namespace lunachess::tests {

std::vector<TestSuite*> g_TestSuites = {
    new PerftTests(),
};

}