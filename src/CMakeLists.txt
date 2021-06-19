SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "-Wall -fPIC")
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-DDEBUG")
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

# INCLUDE_DIRECTORIES(.)
IF (APPLE)
    INCLUDE_DIRECTORIES(/usr/local/include)
    LINK_DIRECTORIES(/usr/local/lib)
ENDIF()

SET(COMP_FILES)
SET(COMP_FILES  ${COMP_FILES}
                Core/EngineImpl.cc Core/Engine.cc Core/WorkerThread.cc
                Core/MemoryBuffer.cc Core/ITC.cc)
SET(COMP_FILES  ${COMP_FILES}
                Job/Job.cc Job/TcpHandlerJob.cc Job/DelayQueue.cc
                Job/HttpClientJob.cc Job/HttpHandlerJob.cc Job/SilentPushJob.cc
                Job/WeComUploadJob.cc)
SET(COMP_FILES  ${COMP_FILES}
                Codec/HttpCodec.cc)
SET(COMP_FILES  ${COMP_FILES}
                WeCom/ServerMessage.cc 
                WeCom/TextServerMessage.cc WeCom/MarkdownServerMessage.cc 
                WeCom/ImageServerMessage.cc WeCom/NewsServerMessage.cc 
                WeCom/MiniProgramServerMessage.cc WeCom/FileServerMessage.cc 
                WeCom/ClientMessage.cc)
SET(COMP_FILES  ${COMP_FILES}
                Utility/Common.cc Utility/Logger.cc Utility/HttpPackage.cc)
SET(COMP_FILES  ${COMP_FILES}
                ThirdParty/tinyxml2/tinyxml2.cpp
                ThirdParty/WXBizMsgCrypt/WXBizMsgCrypt.cpp)
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/md5/md5.cc")
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/cc-base64/base64.cc")
# SET(COMP_FILES ${COMP_FILES} "../ThirdParty/tinyxml2/tinyxml2.cc")

ADD_LIBRARY(wcbot STATIC ${COMP_FILES})
INSTALL(DIRECTORY . DESTINATION include/wcbot FILES_MATCHING PATTERN "*.h")
INSTALL(TARGETS wcbot ARCHIVE DESTINATION lib)
