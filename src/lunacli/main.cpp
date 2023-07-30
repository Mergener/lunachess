#include <iostream>

#include <lunachess.h>

#include <rang/rang.h>

#include "uci.h"

int main() {
    try {
        lunachess::initializeEverything();

        std::ios_base::sync_with_stdio(false);
        std::cin.tie();
        std::cout << std::boolalpha;

        lunachess::Position pos = lunachess::Position::getInitialPosition();

        std::cout << rang::fgB::cyan;
        std::cout << R"( _
| |
| |    _   _ _ __   __ _
| |   | | | | '_ \ / _` |
| |___| |_| | | | | (_| |
\_____/\__,_|_| |_|\__,_|

)"
        << "  by Thomas Mergener" << std::endl
        << "  Version " << LUNA_VERSION_NAME << std::endl;

        std::cout << std::endl << rang::fg::reset << rang::style::reset;

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