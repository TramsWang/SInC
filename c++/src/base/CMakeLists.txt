# Make this sub-directory as a library
add_library(base sinc.cpp)

target_link_libraries(base PRIVATE kb)
target_link_libraries(base PRIVATE util)
target_link_libraries(base PRIVATE rule)
target_link_libraries(base PRIVATE "stdc++fs")

# target_include_directories(
#     base
#     INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
# )