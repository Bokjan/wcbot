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

SET(COMP_FILES  src/Core/EngineImpl.cpp src/Core/Engine.cpp
                src/Core/UvBuffer.cpp src/Core/ITC.cpp src/Core/WorkerThread.cpp
                src/Utility/Common.cpp src/Codec/CodecHttp.cpp)
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/md5/src/md5.cpp")
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/cpp-base64/base64.cpp")
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/tinyxml2/tinyxml2.cpp")

ADD_LIBRARY(wcbot STATIC ${COMP_FILES})
TARGET_LINK_LIBRARIES(wcbot ssl crypto uv)
INSTALL(DIRECTORY src/ DESTINATION include/wcbot FILES_MATCHING PATTERN "*.h")
INSTALL(TARGETS wcbot LIBRARY DESTINATION lib
                      ARCHIVE DESTINATION lib)
