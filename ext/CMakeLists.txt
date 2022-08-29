set(JSON_BuildTests OFF CACHE INTERNAL "")

add_subdirectory(json)

target_include_directories(luna PRIVATE "ext/json/single_include")
