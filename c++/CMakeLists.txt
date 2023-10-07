cmake_minimum_required(VERSION 3.22)
project(Sinc VERSION 3.0 LANGUAGES C CXX)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use `gflags` for parsing command line options and arguments
include(FetchContent)
FetchContent_Declare(
  gflags
  URL https://github.com/gflags/gflags/archive/refs/tags/v2.2.2.zip
)
FetchContent_MakeAvailable(gflags)

# Add all sub-directories
add_subdirectory(src/util)
add_subdirectory(src/kb)
add_subdirectory(src/rule)
add_subdirectory(src/base)
add_subdirectory(src/impl)

# Build SInC executable
add_executable(sinc main.cpp)
target_link_libraries(sinc PRIVATE util)
target_link_libraries(sinc PRIVATE kb)
target_link_libraries(sinc PRIVATE rule)
target_link_libraries(sinc PRIVATE base)
target_link_libraries(sinc PRIVATE impl)
target_link_libraries(sinc PRIVATE gflags::gflags)
target_link_libraries(sinc PRIVATE "stdc++fs")  # Use namespace `std::filesystem`

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#   # Use debug_new only in debug mode
#   target_link_libraries(sinc PRIVATE debug_new)
# endif()

# Use GoogleTest for testing
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()
add_executable(test_sinc
  test_sinc.cpp
  test/util/common_test.cpp
  test/util/util_test.cpp
  test/util/graphAlg_test.cpp
  test/kb/testKbUtils.cpp
  test/kb/intTable_test.cpp
  test/kb/simpleKb_test.cpp
  test/rule/components_test.cpp
  test/rule/rule_test.cpp
  test/base/sinc_test.cpp
  test/impl/sincWithCache_test.cpp
  test/impl/app_test.cpp
)

target_link_libraries(test_sinc PRIVATE util)
target_link_libraries(test_sinc PRIVATE kb)
target_link_libraries(test_sinc PRIVATE rule)
target_link_libraries(test_sinc PRIVATE base)
target_link_libraries(test_sinc PRIVATE impl)
target_link_libraries(test_sinc PRIVATE GTest::gtest_main)
# target_link_libraries(test_sinc PRIVATE gflags::gflags)
target_link_libraries(test_sinc PRIVATE "stdc++fs")  # Use namespace `std::filesystem`

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     target_link_libraries(test_sinc PRIVATE debug_new)
# endif()

include(GoogleTest)
gtest_discover_tests(test_sinc)

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)
