cmake_minimum_required(VERSION 3.12)
project(luna)

##
# Luna version numbers
##
set(LUNA_VERSION_MAJOR "0")
set(LUNA_VERSION_MINOR "4")
set(LUNA_VERSION_PATCH "0")

message("Luna version is set to ${LUNA_VERSION_MAJOR}.${LUNA_VERSION_MINOR}.${LUNA_VERSION_PATCH}")

##
# Executable name
##
set(LUNA_OUT_NAME "luna")

##
# Project settings
##
set(CMAKE_CXX_STANDARD 17)

##
# Source code
##
add_library(luna STATIC
        src/luna/types.cpp
        src/luna/bitboard.cpp
        src/luna/chessgame.cpp
        src/luna/endgame.cpp
        src/luna/piece.cpp
        src/luna/position.cpp
        src/luna/zobrist.cpp
        src/luna/square.cpp
        src/luna/move.cpp
        src/luna/perft.cpp
        src/luna/lunachess.cpp
        src/luna/debug.cpp
        src/luna/staticanalysis.cpp
        src/luna/strutils.cpp
        src/luna/ai/timemanager.cpp
        src/luna/ai/search.cpp
        src/luna/ai/transpositiontable.cpp
        src/luna/ai/aimovefactory.cpp
        src/luna/ai/quiescevaluator.cpp
        src/luna/utils.cpp
        src/luna/endgame.cpp
        src/luna/endgame.h src/luna/openingbook.cpp
        src/luna/openingbook.h src/luna/pst.cpp
        src/luna/pst.h
        src/luna/ai/hce/hce.cpp
        src/luna/ai/hce/hce.h
        src/luna/ai/aimovefactory.h
        src/luna/ai/evaluator.h
        src/luna/ai/quiescevaluator.h
        src/luna/ai/search.h
        src/luna/ai/timemanager.h
        src/luna/ai/transpositiontable.h
        src/luna/bitboard.h
        src/luna/bits.h
        src/luna/chessgame.h
        src/luna/clock.h
        src/luna/debug.h
        src/luna/endgame.h
        src/luna/lunachess.h
        src/luna/move.h
        src/luna/movegen.h
        src/luna/openingbook.h
        src/luna/perft.h
        src/luna/piece.h
        src/luna/position.h
        src/luna/pst.h
        src/luna/staticanalysis.h
        src/luna/staticlist.h
        src/luna/strutils.h
        src/luna/types.h
        src/luna/utils.h
        src/luna/zobrist.h
        src/luna/threadpool.h src/luna/threadpool.cpp src/luna/threadpool.h src/luna/ai/hce/hceweights.cpp src/luna/ai/hce/hceweights.h)

add_executable(lunacli
        src/lunacli/main.cpp
        src/lunacli/uci.cpp
        src/lunacli/uci.h)

add_executable(lunatest
        src/lunatest/main.cpp
        src/lunatest/testlist.cpp)

##
# Compilation settings
##
set_target_properties(luna PROPERTIES OUTPUT_NAME ${LUNA_OUT_NAME})
set_target_properties(lunacli PROPERTIES OUTPUT_NAME ${LUNA_OUT_NAME})
target_compile_definitions(lunacli PUBLIC LUNA_VERSION_MAJOR=${LUNA_VERSION_MAJOR})
target_compile_definitions(lunacli PUBLIC LUNA_VERSION_MINOR=${LUNA_VERSION_MINOR})
target_compile_definitions(lunacli PUBLIC LUNA_VERSION_PATCH=${LUNA_VERSION_PATCH})
target_compile_definitions(lunatest PUBLIC LUNA_VERSION_MAJOR=${LUNA_VERSION_MAJOR})
target_compile_definitions(lunatest PUBLIC LUNA_VERSION_MINOR=${LUNA_VERSION_MINOR})
target_compile_definitions(lunatest PUBLIC LUNA_VERSION_PATCH=${LUNA_VERSION_PATCH})

##
# Add external dependencies
##
add_subdirectory("ext")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC requires some flags in order for threads and <filesystem> to work.
    set(CMAKE_CXX_FLAGS "-pthread -static-libstdc++ -static-libgcc --static")
    target_link_libraries(luna PUBLIC stdc++fs
            PUBLIC nlohmann_json)
else()
    target_link_libraries(luna PUBLIC nlohmann_json)
endif()

target_link_libraries(lunacli PRIVATE luna)
target_link_libraries(lunatest PRIVATE luna)

target_include_directories(luna PUBLIC "ext/json/single_include")
target_include_directories(lunacli PUBLIC "ext/json/single_include" "src/luna")
target_include_directories(lunatest PUBLIC "ext/json/single_include" "src/luna")