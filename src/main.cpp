#include <iostream>
#include <thread>

#include "lunachess.h"

using namespace lunachess;

int main(int argc, char* argv[]) {
    try {
        lunachess::initializeEverything();

        std::ios_base::sync_with_stdio(false);
        std::cin.tie();
        std::cout << std::boolalpha;

        return uciMain();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw e;
    }
}