#include <iostream>

#include <lunachess.h>

#include "uci.h"

int main() {
    try {
        lunachess::initializeEverything();

        std::ios_base::sync_with_stdio(false);
        std::cin.tie();
        std::cout << std::boolalpha;

        std::cout << "LunaChess AB " << LUNA_VERSION_NAME << std::endl;

#ifdef LUNA_ASSERTS_ON
        std::cout << "LUNA_ASSERTS_ON is enabled." << std::endl;
#endif
#ifndef NDEBUG
        std::cout << "This is a Debug build. Search/Perft times may be considerably slower." << std::endl;
#endif

        return lunachess::uciMain();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw e;
    }
}