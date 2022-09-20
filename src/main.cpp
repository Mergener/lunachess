#include <iostream>
#include <thread>

#include "lunachess.h"

int main() {
    try {
        lunachess::initializeEverything();

        std::ios_base::sync_with_stdio(false);
        std::cin.tie();
        std::cout << std::boolalpha;

        std::cout << "LunaChess AB v" << LUNA_VERSION_MAJOR << "." << LUNA_VERSION_MINOR << "." << LUNA_VERSION_PATCH << std::endl;

#ifdef LUNA_ASSERTS_ON
        std::cout << "LUNA_ASSERTS_ON is enabled." << std::endl;
#endif

        return lunachess::uciMain();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw e;
    }
}