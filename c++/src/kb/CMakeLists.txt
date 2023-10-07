# Make this sub-directory as a library
add_library(kb intTable.cpp simpleKb.cpp)

target_link_libraries(kb PRIVATE util)
target_link_libraries(kb PRIVATE gflags::gflags)

# target_include_directories(
#     kb
#     INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
# )