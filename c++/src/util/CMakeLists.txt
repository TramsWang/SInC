# Make this sub-directory as a library
add_library(util common.cpp util.cpp graphAlg.cpp)

# target_include_directories(
#     util
#     INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
# )