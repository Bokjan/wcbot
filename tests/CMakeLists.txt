cmake_minimum_required(VERSION 3.14)
project(wcbot_tests CXX)

# GoogleTest is fetched at configure time so contributors do not need a
# system-wide installation. Pin to a recent stable release.
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
    URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
)
# Avoid GoogleTest installing itself when the parent project does `install`.
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)
enable_testing()

# All tests link the wcbot static library and gtest_main. They are kept
# deliberately framework-light: anything that requires `worker_impl::g_ThisThread`
# (Job::Sleep, IOJob::ArmTimeout, TcpHandlerJob::SendData) is exercised via
# code paths that do not actually invoke those entry points.
set(WCBOT_TEST_SOURCES
    Common_test.cc
    MemoryBuffer_test.cc
    CronTrigger_test.cc
    HttpPackage_test.cc
    HttpCodec_test.cc
    TimeQueue_test.cc
    TimeWheel_test.cc
    Job_test.cc
)

add_executable(wcbot_tests ${WCBOT_TEST_SOURCES})

target_compile_options(wcbot_tests PRIVATE -Wall)

# Tests are part of this repo and reach into internal headers (e.g.
# `Core/TimeQueue.h`, `Codec/HttpCodec.h`) that are intentionally not part of
# the installed public API. Expose `src/` as a private include path here so
# tests can use short includes without polluting the public surface.
target_include_directories(wcbot_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(wcbot_tests PRIVATE
    wcbot
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(wcbot_tests)
