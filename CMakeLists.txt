cmake_minimum_required(VERSION 3.12)

##
# Luna version numbers
##
set(LUNA_VERSION_MAJOR "0")
set(LUNA_VERSION_MINOR "2")
set(LUNA_VERSION_PATCH "0")

message("Luna version is set to ${LUNA_VERSION_MAJOR}.${LUNA_VERSION_MINOR}.${LUNA_VERSION_PATCH}")

##
# Executable name
##
set(LUNA_EXEC_NAME "luna_${LUNA_VERSION_MAJOR}.${LUNA_VERSION_MINOR}.${LUNA_VERSION_PATCH}")

##
# Project settings
##
project(luna)
set(CMAKE_CXX_STANDARD 17)

##
# Source code
##
add_executable(luna
        src/main.cpp
        src/types.cpp
        src/bitboard.cpp
        src/chessgame.cpp
        src/endgame.cpp
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
        src/ai/classiceval/hotmap.cpp
        src/ai/neuraleval/neuraleval.cpp
        src/ai/classiceval/texel.cpp
        src/ai/timemanager.cpp
        src/ai/classiceval/classicevaluator.cpp
        src/ai/search.cpp
        src/ai/transpositiontable.cpp
        src/ai/aimovefactory.cpp
        src/ai/classiceval/aibitboards.cpp
        src/ai/quiescevaluator.cpp
        src/uci.cpp
        src/ai/neuraleval/nn.cpp
        src/utils.cpp src/ai/neuraleval/neuralgenetic.cpp src/endgame.cpp src/endgame.h)

##
# Compilation settings
##
set_target_properties(luna PROPERTIES OUTPUT_NAME ${LUNA_EXEC_NAME})
target_compile_definitions(luna PUBLIC LUNA_VERSION_MAJOR=${LUNA_VERSION_MAJOR})
target_compile_definitions(luna PUBLIC LUNA_VERSION_MINOR=${LUNA_VERSION_MINOR})
target_compile_definitions(luna PUBLIC LUNA_VERSION_PATCH=${LUNA_VERSION_PATCH})

##
# Add external dependencies
##
add_subdirectory("ext")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC requires some flags in order for threads and <filesystem> to work.
    set(CMAKE_CXX_FLAGS "-pthread -static-libstdc++ -static-libgcc")
    target_link_libraries(luna PRIVATE stdc++fs
                               PRIVATE nlohmann_json)
else()
    target_link_libraries(luna PRIVATE nlohmann_json)
endif()

