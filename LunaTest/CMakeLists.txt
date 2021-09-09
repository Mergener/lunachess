set(HEADERS
    "src/tests.h"
)

set(SRC
    "src/testarray.cpp"
    "src/testmain.cpp"
    "src/tests/bitboards.cpp"
    "src/tests/perft.cpp"
    "src/tests/pieces.cpp"
    "src/tests/positions.cpp"
    "src/tests/staticlist.cpp"
)

set(ALL
    ${HEADERS}
    ${SRC}
)

add_executable(lunatest ${ALL})

set_property(TARGET lunatest PROPERTY CXX_STANDARD 17)

target_include_directories(lunatest PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../LunaChess/src"
)

target_compile_definitions(lunatest PRIVATE
    "LUNA_ASSERTS_ON;"
)

add_dependencies(lunatest
    lunachess
)

target_link_libraries(lunatest PRIVATE
    lunachess
)

