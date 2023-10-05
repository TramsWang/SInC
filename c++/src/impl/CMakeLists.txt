# Make this sub-directory as a library
add_library(impl sincWithCache.cpp)

target_link_libraries(impl PRIVATE kb)
target_link_libraries(impl PRIVATE util)
target_link_libraries(impl PRIVATE rule)
target_link_libraries(impl PRIVATE base)

# target_include_directories(
#     base
#     INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
# )