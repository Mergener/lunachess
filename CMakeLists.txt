cmake_minimum_required(VERSION 3.12)

# Luna version numbers
set(LUNA_VERSION_MAJOR "0")
set(LUNA_VERSION_MINOR "1")
set(LUNA_VERSION_PATCH "0")

# Executable name
set(LUNA_EXEC_NAME "luna_${LUNA_VERSION_MAJOR}.${LUNA_VERSION_MINOR}.${LUNA_VERSION_PATCH}")

message(${LUNA_EXEC_NAME})

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
        src/ai/hotmap.cpp
        src/ai/neural/neuraleval.cpp
        src/ai/timemanager.cpp
        src/ai/classicevaluator.cpp
        src/ai/search.cpp
        src/ai/transpositiontable.cpp
        src/ai/aimovefactory.cpp
        src/ai/aibitboards.cpp
        src/ai/quiescevaluator.cpp
        src/uci.cpp src/ai/neural/nn.cpp src/ai/neural/nn.h src/utils.cpp src/ai/neural/neuralgenetic.cpp src/ai/neural/neuralgenetic.h)

set_target_properties(luna PROPERTIES OUTPUT_NAME ${LUNA_EXEC_NAME})
target_compile_definitions(luna PUBLIC LUNA_VERSION_MAJOR=${LUNA_VERSION_MAJOR})
target_compile_definitions(luna PUBLIC LUNA_VERSION_MINOR=${LUNA_VERSION_MINOR})
target_compile_definitions(luna PUBLIC LUNA_VERSION_PATCH=${LUNA_VERSION_PATCH})

# GCC requires flags in order for threads and <filesystem> to work.

add_subdirectory("ext")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-pthread")
    target_link_libraries(luna PRIVATE stdc++fs
                               PRIVATE nlohmann_json)
else()
    target_link_libraries(luna PRIVATE nlohmann_json)
endif()

