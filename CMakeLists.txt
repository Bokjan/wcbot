CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(wcbot)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "-Wall -fPIC")
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-DDEBUG")
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

INCLUDE_DIRECTORIES(./include ./src)
IF (APPLE)
    INCLUDE_DIRECTORIES(/usr/local/include)
ENDIF()

LINK_LIBRARIES(pthread openssl uv)

SET(COMP_FILES  src/Core/EngineImpl.cc src/Core/Engine.cc src/Codec/CodecHttp.cc
                src/Core/MemoryBuffer.cc src/Core/ITC.cc src/Core/WorkerThread.cc
                src/Job/TcpHandlerJob.cc src/Job/DelayQueue.cc
                src/Utility/Common.cc src/Utility/Logger.cc)
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/md5/src/md5.cc")
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/cc-base64/base64.cc")
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/tinyxml2/tinyxml2.cc")

ADD_LIBRARY(wcbot STATIC ${COMP_FILES})
TARGET_LINK_LIBRARIES(wcbot ssl crypto uv)
INSTALL(DIRECTORY src/ DESTINATION include/wcbot FILES_MATCHING PATTERN "*.h")
INSTALL(TARGETS wcbot LIBRARY DESTINATION lib
                      ARCHIVE DESTINATION lib)
