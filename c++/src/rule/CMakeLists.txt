# Make this sub-directory as a library
add_library(rule components.cpp)

target_include_directories(
    rule
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)