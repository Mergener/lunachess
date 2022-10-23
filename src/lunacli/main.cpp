#include <iostream>
#include <thread>

#include <lunachess.h>

#include "uci.h"

int main() {
    try {
        lunachess::initializeEverything();

        std::ios_base::sync_with_stdio(false);
        std::cin.tie();
        std::cout << std::boolalpha;

        lunachess::ai::neural::NNLayer<4, 3> nnLayer = j;
        std::cout << nlohmann::json(nnLayer) << std::endl;

        //test();

        std::cout << "LunaChess AB v" << LUNA_VERSION_MAJOR << "." << LUNA_VERSION_MINOR << "." << LUNA_VERSION_PATCH << std::endl;

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