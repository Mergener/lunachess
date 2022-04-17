cmake_minimum_required(VERSION 3.14)

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
        src/ai/genetic.cpp
        src/ai/neural/nn.cpp
        src/ai/timemanager.cpp
        src/ai/basicevaluator.cpp
        src/ai/search.cpp
        src/ai/transpositiontable.cpp
        src/ai/aimovegen.cpp
        src/ai/aibitboards.cpp
        src/uci.cpp)

# GCC requires flags in order for threads and <filesystem> to work.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-pthread -Wall")
    target_link_libraries(luna stdc++fs)
endif()