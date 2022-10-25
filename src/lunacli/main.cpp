#include <iostream>
#include <thread>

#include <lunachess.h>

#include "uci.h"

void test() {
    using namespace lunachess;
    
    using TLayer      = ai::neural::NNLayer<4, 3>;
    using InputArray  = TLayer::InputArray;
    using OutputArray = TLayer::OutputArray;

    // Create and zero arrays
    InputArray inputs;
    std::fill(inputs.begin(), inputs.end(), 1);
    OutputArray outputs;
    std::fill(outputs.begin(), outputs.end(), 0);

    // Initialize layer
    TLayer layer = nlohmann::json::parse(R"({"weights": [[1,1,1], [1,1,1], [1,1,1], [1,1,1]], "biases": [0,0,0,0]})");
    std::cout << "INPUT_ARRAY_SIZE: " << TLayer::INPUT_ARRAY_SIZE << std::endl;

    std::cout << "Layer: " << nlohmann::json(layer) << std::endl;

    // Perform propagation and output results
    std::cout << "Inputs: [ ";
    for (int i = 0; i < inputs.size(); ++i) {
        std::cout << inputs[i] << " ";
    }
    std::cout << "]" << std::endl;

    layer.propagate(inputs, outputs);
    
    std::cout << "Outputs: [ ";
    for (int i = 0; i < outputs.size(); ++i) {
        std::cout << outputs[i] << " ";
    }
    std::cout << "]" << std::endl;
}

int main() {
    try {
        lunachess::initializeEverything();

        std::ios_base::sync_with_stdio(false);
        std::cin.tie();
        std::cout << std::boolalpha;

        test();

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