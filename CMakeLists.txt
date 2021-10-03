cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

project(lunachess CXX)

set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lwsock32 -lws2_32 ${CMAKE_CXX_STANDARD_LIBRARIES}")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")

add_subdirectory(ext/json)
set(NLOHMANN_JSON_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/ext/json/single_include/nlohmann)

add_subdirectory(LunaChess)
add_subdirectory(LunaChessCLI)
add_subdirectory(LunaTest)