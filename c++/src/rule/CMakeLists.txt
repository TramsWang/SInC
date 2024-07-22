# Make this sub-directory as a library
add_library(rule components.cpp rule.cpp)

target_link_libraries(rule PRIVATE util)

# target_include_directories(
#     rule
#     INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
# )