#include "lunatest.h"

#include <vector>

#include "tests/movegen/perfttests.cpp"
#include "tests/template.cpp"

namespace lunachess::tests {

std::vector<TestSuite*> g_TestSuites;

void createTests() {
    g_TestSuites = {
        new PerftTests(),
    };
}

}