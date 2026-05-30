cmake_minimum_required(VERSION 3.10...3.30)
project(wcbotd CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE_UPPER)
if(_BUILD_TYPE_UPPER STREQUAL "DEBUG")
    add_compile_definitions(DEBUG)
endif()

set(SAMPLE_SOURCES
    main.cc
    Job/QBJob.cc
    Job/EchoCallbackJob.cc
)

add_executable(wcbotd ${SAMPLE_SOURCES})

target_compile_options(wcbotd PRIVATE -Wall -fPIC)

# Sample's own headers (e.g. `Job/QBJob.h`).
target_include_directories(wcbotd PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# Two supported build modes:
#
#   1. Configured from the top-level `CMakeLists.txt` (recommended):
#      The `wcbot` target is already defined; we simply link against it and
#      inherit all transitive dependencies (libuv/curl/openssl/pthread) and
#      include directories.
#
#   2. Configured standalone from `sample/` (e.g. `cd sample && cmake -B build`):
#      The `wcbot` target does not exist here, so we must locate the
#      installed/built `libwcbot.a` and re-discover its dependencies
#      ourselves. Set `-DWCBOT_ROOT=/path/to/install_or_build_root` if it is
#      not installed system-wide.
# ---------------------------------------------------------------------------
if(TARGET wcbot)
    target_link_libraries(wcbotd PRIVATE wcbot)
else()
    message(STATUS "wcbot target not found; configuring sample in standalone mode")

    # Locate libwcbot.a + public headers.
    find_library(WCBOT_LIBRARY
        NAMES wcbot
        HINTS "${WCBOT_ROOT}/lib"
              "${CMAKE_CURRENT_SOURCE_DIR}/../build/src"
              "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib"
    )
    find_path(WCBOT_INCLUDE_DIR
        NAMES wcbot/Core/Engine.h
        HINTS "${WCBOT_ROOT}/include"
              "${CMAKE_CURRENT_SOURCE_DIR}/../build/include"
    )
    if(NOT WCBOT_LIBRARY OR NOT WCBOT_INCLUDE_DIR)
        message(FATAL_ERROR
            "Could not locate libwcbot.a / wcbot public headers.\n"
            "Either build from the top-level CMakeLists.txt, or pass\n"
            "  -DWCBOT_ROOT=<install-or-build-root>\n"
            "(checked: ${WCBOT_LIBRARY} / ${WCBOT_INCLUDE_DIR})")
    endif()

    # Re-discover the transitive dependencies that `libwcbot.a` itself uses.
    find_package(OpenSSL REQUIRED)
    find_package(CURL REQUIRED)
    find_package(Threads REQUIRED)
    find_path(LIBUV_INCLUDE_DIR NAMES uv.h)
    find_library(LIBUV_LIBRARY NAMES uv libuv)
    if(NOT LIBUV_INCLUDE_DIR OR NOT LIBUV_LIBRARY)
        message(FATAL_ERROR "libuv not found (uv.h / libuv).")
    endif()

    target_include_directories(wcbotd PRIVATE
        "${WCBOT_INCLUDE_DIR}"
        "${LIBUV_INCLUDE_DIR}"
    )
    target_link_libraries(wcbotd PRIVATE
        "${WCBOT_LIBRARY}"
        "${LIBUV_LIBRARY}"
        OpenSSL::Crypto
        OpenSSL::SSL
        CURL::libcurl
        Threads::Threads
    )
endif()
