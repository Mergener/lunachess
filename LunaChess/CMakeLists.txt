set(HEADERS
    "src/ai/aibitboards.h"
    "src/ai/basicevaluator.h"
    "src/ai/evaluator.h"
    "src/ai/movepicker.h"
    "src/ai/openingbook.h"
    "src/ai/scorestable.h"
    "src/ai/tranpositiontable.h"
    "src/core/bitboard.h"
    "src/core/bits.h"
    "src/core/debug.h"
    "src/core/defs.h"
    "src/core/move.h"
    "src/core/piece.h"
    "src/core/position.h"
    "src/core/square.h"
    "src/core/staticlist.h"
    "src/core/types.h"
    "src/core/zobrist.h"
    "src/lunachess.h"
)

set(SRC
    "src/ai/aibitboards.cpp"
    "src/ai/basicevaluator.cpp"
    "src/ai/movepicker.cpp"
    "src/ai/scorestable.cpp"
    "src/ai/transpositiontable.cpp"
    "src/core/bitboard.cpp"
    "src/core/debug.cpp"
    "src/core/lunachess.cpp"
    "src/core/move.cpp"
    "src/core/piece.cpp"
    "src/core/position.cpp"
    "src/core/square.cpp"
    "src/core/zobrist.cpp"
        )

set(ALL
    ${HEADERS}
    ${SRC})

add_library(lunachess STATIC ${ALL})

set_target_properties(lunachess PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS ON
        )

target_include_directories(lunachess PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
