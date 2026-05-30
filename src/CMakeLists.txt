cmake_minimum_required(VERSION 3.10...3.30)

# ---------- compiler & flags ----------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Honor the `BUILD_TYPE` regardless of case: previous code only matched
# uppercase "DEBUG", which silently failed for the more common "Debug".
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE_UPPER)
if(_BUILD_TYPE_UPPER STREQUAL "DEBUG")
    add_compile_definitions(DEBUG)
endif()

# ---------- dependency discovery ----------
# OpenSSL: provides libcrypto / libssl
find_package(OpenSSL REQUIRED)

# cURL: prefer the imported target form; fall back to plain variables on older
# CMake. We only require libcurl, no protocol-specific options here.
find_package(CURL REQUIRED)

# libuv: not all distributions ship a CMake package config, so probe manually.
find_path(LIBUV_INCLUDE_DIR NAMES uv.h)
find_library(LIBUV_LIBRARY NAMES uv libuv)
if(NOT LIBUV_INCLUDE_DIR OR NOT LIBUV_LIBRARY)
    message(FATAL_ERROR "libuv not found. Please install libuv (uv.h + libuv).")
endif()

# RapidJSON is header-only; locate its include path.
find_path(RAPIDJSON_INCLUDE_DIR NAMES rapidjson/document.h)
if(NOT RAPIDJSON_INCLUDE_DIR)
    message(FATAL_ERROR "RapidJSON headers not found. Please install rapidjson.")
endif()

# Threads (pthread) for std::thread/std::mutex on Linux.
find_package(Threads REQUIRED)

# ---------- source list ----------
set(WCBOT_SOURCES
    Core/Engine.cc
    Core/EngineImpl.cc
    Core/WorkerThread.cc
    Core/ITC.cc
    Core/TimeWheel.cc
    Core/TimeQueue.cc

    Job/Job.cc
    Job/TcpHandlerJob.cc
    Job/HttpClientJob.cc
    Job/HttpHandlerJob.cc
    Job/SilentPushJob.cc
    Job/WeComUploadJob.cc
    Job/MessageCallbackJob.cc
    Job/GetChatInfoJob.cc

    Codec/HttpCodec.cc

    WeCom/ServerMessage.cc
    WeCom/TextServerMessage.cc
    WeCom/MarkdownServerMessage.cc
    WeCom/ImageServerMessage.cc
    WeCom/NewsServerMessage.cc
    WeCom/MiniProgramServerMessage.cc
    WeCom/FileServerMessage.cc
    WeCom/ClientMessage.cc
    WeCom/TextClientMessage.cc
    WeCom/ImageClientMessage.cc
    WeCom/EventClientMessage.cc
    WeCom/AttachmentClientMessage.cc
    WeCom/MixedClientMessage.cc
    WeCom/ClientMessageImpl.cc

    Utility/Common.cc
    Utility/Logger.cc
    Utility/HttpPackage.cc
    Utility/MemoryBuffer.cc
    Utility/CronTrigger.cc
    Utility/SyncFileLogger.cc

    ThirdParty/tinyxml2/tinyxml2.cpp
    ThirdParty/WXBizMsgCrypt/WXBizMsgCrypt.cpp
)

# ---------- target ----------
add_library(wcbot STATIC ${WCBOT_SOURCES})

target_compile_options(wcbot PRIVATE -Wall -fPIC)

# Public include paths so downstream targets get them transitively.
target_include_directories(wcbot
    PUBLIC
        ${LIBUV_INCLUDE_DIR}
        ${RAPIDJSON_INCLUDE_DIR}
)

# Build-time public headers under `${CMAKE_BINARY_DIR}/include/wcbot/...` so
# that downstream targets (e.g. the `sample/` executable) can use the same
# `#include "wcbot/Module/Header.h"` paths whether or not the library has
# been installed. We mirror the same headers exposed by the install tree.
set(WCBOT_PUBLIC_HEADER_DIR "${CMAKE_BINARY_DIR}/include/wcbot")
set(_WCBOT_HEADER_PAIRS
    Core/Engine.h
    Codec/Codec.h
    Utility/Common.h
    Utility/CronTrigger.h
    Utility/HttpPackage.h
    Utility/Logger.h
    Utility/MemoryBuffer.h
    Job/Job.h
    Job/HttpClientJob.h
    Job/SilentPushJob.h
    Job/WeComUploadJob.h
    Job/MessageCallbackJob.h
    Job/GetChatInfoJob.h
    WeCom/ServerMessage.h
    WeCom/ClientMessage.h
    WeCom/TextServerMessage.h
    WeCom/MarkdownServerMessage.h
    WeCom/ImageServerMessage.h
    WeCom/NewsServerMessage.h
    WeCom/FileServerMessage.h
    WeCom/MiniProgramServerMessage.h
    WeCom/TextClientMessage.h
    WeCom/ImageClientMessage.h
    WeCom/EventClientMessage.h
    WeCom/AttachmentClientMessage.h
    WeCom/MixedClientMessage.h
)
foreach(_h IN LISTS _WCBOT_HEADER_PAIRS)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${_h}"
                   "${WCBOT_PUBLIC_HEADER_DIR}/${_h}" COPYONLY)
endforeach()

target_include_directories(wcbot PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

# Link dependencies via imported targets where possible.
target_link_libraries(wcbot
    PUBLIC
        ${LIBUV_LIBRARY}
        OpenSSL::Crypto
        OpenSSL::SSL
        CURL::libcurl
        Threads::Threads
)

# ---------- install ----------
set(INSTALL_HEADERS_CORE    Core/Engine.h)
set(INSTALL_HEADERS_CODEC   Codec/Codec.h)
set(INSTALL_HEADERS_UTILITY Utility/Common.h Utility/CronTrigger.h Utility/HttpPackage.h
                            Utility/Logger.h Utility/MemoryBuffer.h)
set(INSTALL_HEADERS_JOB     Job/Job.h Job/HttpClientJob.h Job/SilentPushJob.h
                            Job/WeComUploadJob.h Job/MessageCallbackJob.h Job/GetChatInfoJob.h)
set(INSTALL_HEADERS_WECOM   WeCom/ServerMessage.h WeCom/ClientMessage.h
                            WeCom/TextServerMessage.h WeCom/MarkdownServerMessage.h
                            WeCom/ImageServerMessage.h WeCom/NewsServerMessage.h
                            WeCom/FileServerMessage.h WeCom/MiniProgramServerMessage.h
                            WeCom/TextClientMessage.h WeCom/ImageClientMessage.h
                            WeCom/EventClientMessage.h WeCom/AttachmentClientMessage.h
                            WeCom/MixedClientMessage.h)

install(FILES ${INSTALL_HEADERS_CORE}    DESTINATION include/wcbot/Core)
install(FILES ${INSTALL_HEADERS_CODEC}   DESTINATION include/wcbot/Codec)
install(FILES ${INSTALL_HEADERS_JOB}     DESTINATION include/wcbot/Job)
install(FILES ${INSTALL_HEADERS_WECOM}   DESTINATION include/wcbot/WeCom)
install(FILES ${INSTALL_HEADERS_UTILITY} DESTINATION include/wcbot/Utility)

install(TARGETS wcbot ARCHIVE DESTINATION lib)
