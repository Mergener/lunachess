cmake_minimum_required(VERSION 3.12)

# Project settings
project(luna)
set(CMAKE_CXX_STANDARD 17)

# Source code
add_executable(luna
        src/main.cpp
        src/types.cpp
        src/bitboard.cpp
        src/piece.cpp
        src/position.cpp
        src/zobrist.cpp
        src/square.cpp
        src/move.cpp
        src/perft.cpp
        src/lunachess.cpp
        src/debug.cpp
        src/uci.h
        src/posutils.cpp
        src/strutils.cpp
        src/serial.cpp
        src/ai/hotmap.cpp
        src/ai/neural/neuraleval.cpp
        src/ai/timemanager.cpp
        src/ai/basicevaluator.cpp
        src/ai/search.cpp
        src/ai/transpositiontable.cpp
        src/ai/aimovefactory.cpp
        src/ai/aibitboards.cpp
        src/uci.cpp src/ai/neural/nn.cpp src/ai/neural/nn.h src/utils.cpp src/ai/neural/neuralgenetic.cpp src/ai/neural/neuralgenetic.h src/ai/quiescevaluator.cpp src/ai/quiescevaluator.h)

# GCC requires flags in order for threads and <filesystem> to work.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-pthread")
endif()

add_subdirectory("ext")


target_link_libraries(luna PRIVATE stdc++fs
                           PRIVATE nlohmann_json)