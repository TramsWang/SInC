# Make this sub-directory as a library
add_library(kb intTable.cpp simpleKb.cpp)

target_include_directories(
    kb
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)