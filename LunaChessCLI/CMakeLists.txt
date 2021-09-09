set(HEADERS
    "src/app.h"
    "src/appmode.h"
    "src/console.h"
    "src/error.h"
    "src/evalmode.h"
    "src/perftmode.h"
    "src/playmode.h"
)

set(SRC
    "src/app.cpp"
    "src/console.cpp"
    "src/evalmode.cpp"
    "src/perftmode.cpp"
    "src/playmode.cpp"
)

set(ALL
    ${HEADERS}
    ${SRC}
)

add_executable(lunachesscli ${ALL})

set_target_properties(lunachesscli PROPERTIES OUTPUT_NAME lunachess)

target_include_directories(lunachesscli PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../LunaChess/src"
)

add_dependencies(lunachesscli
    lunachess
)

set_property(TARGET lunachesscli PROPERTY CXX_STANDARD 17)

target_link_libraries(lunachesscli PRIVATE
    lunachess
)