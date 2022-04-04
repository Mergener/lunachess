cmake_minimum_required(VERSION 3.14)
project(luna)

set(CMAKE_CXX_STANDARD 17)

add_executable(luna src/main.cpp src/types.h src/bits.h src/piece.h src/types.cpp src/bitboard.h src/bitboard.cpp src/piece.cpp src/position.cpp src/position.h src/zobrist.cpp src/zobrist.h src/square.cpp src/move.cpp src/move.h src/staticlist.h src/utils.h src/perft.h src/perft.cpp src/lunachess.cpp src/lunachess.h src/debug.cpp src/debug.h src/ai/evaluator.h src/ai/transpositiontable.h src/ai/basicevaluator.h src/ai/basicevaluator.cpp src/ai/search.h src/lock.h src/ai/search.cpp src/ai/transpositiontable.cpp src/ai/aimovegen.cpp src/ai/aimovegen.h src/ai/aibitboards.cpp src/ai/aibitboards.h src/uci.cpp src/uci.h src/strutils.cpp src/strutils.h src/ai/hotmap.cpp src/ai/hotmap.h src/ai/genetic.cpp src/ai/genetic.h src/serial.cpp src/serial.h src/clock.h src/ai/neural/nn.cpp src/ai/neural/nn.h src/movegen.h src/posutils.h src/ai/timemanager.cpp src/ai/timemanager.h)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-pthread --std=c++17 -lstdc++fs")
    target_link_libraries(luna stdc++fs)
endif()