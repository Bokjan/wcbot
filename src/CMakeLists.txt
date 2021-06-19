SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "-Wall -fPIC")
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-DDEBUG")
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

INCLUDE_DIRECTORIES(.)
IF (APPLE)
    INCLUDE_DIRECTORIES(/usr/local/include)
ENDIF()

LINK_LIBRARIES(pthread openssl uv)

SET(COMP_FILES  Core/EngineImpl.cc Core/Engine.cc Core/WorkerThread.cc
                Core/MemoryBuffer.cc Core/ITC.cc 
                Job/Job.cc Job/TcpHandlerJob.cc Job/DelayQueue.cc
                Job/HttpClientJob.cc Job/HttpHandlerJob.cc
                Codec/HttpCodec.cc
                Utility/Common.cc Utility/Logger.cc Utility/HttpPackage.cc)
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/md5/md5.cc")
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/cc-base64/base64.cc")
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/tinyxml2/tinyxml2.cc")

ADD_LIBRARY(wcbot STATIC ${COMP_FILES})
TARGET_LINK_LIBRARIES(wcbot ${CURL_LIBRARIES})
INSTALL(DIRECTORY . DESTINATION include/wcbot FILES_MATCHING PATTERN "*.h")
INSTALL(TARGETS wcbot ARCHIVE DESTINATION lib)
