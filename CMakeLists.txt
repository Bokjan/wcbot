cmake_minimum_required(VERSION 3.10...3.30)
project(wcbot CXX)

# Default to Release if the user did not specify a build type, so optimizations
# are on by default.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

option(WCBOT_BUILD_SAMPLE "Build the sample executable under sample/" ON)
option(WCBOT_BUILD_TESTS  "Build unit tests under tests/ (requires network for FetchContent)" OFF)

add_subdirectory(src)

if(WCBOT_BUILD_SAMPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sample/CMakeLists.txt")
    add_subdirectory(sample)
endif()

if(WCBOT_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()
