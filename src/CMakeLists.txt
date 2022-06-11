SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "-Wall -fPIC")
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-DDEBUG")
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

# INCLUDE_DIRECTORIES(.)
IF (APPLE)
    INCLUDE_DIRECTORIES(/usr/local/include)
    LINK_DIRECTORIES(/usr/local/lib)
    INCLUDE_DIRECTORIES(/usr/local/opt/openssl/include)
    LINK_DIRECTORIES(/usr/local/opt/openssl/lib)
ENDIF()

SET(COMP_FILES)
SET(COMP_FILES  ${COMP_FILES}
                Core/EngineImpl.cc Core/Engine.cc Core/WorkerThread.cc
                Core/ITC.cc Core/TimeWheel.cc Core/TimeQueue.cc)
SET(COMP_FILES  ${COMP_FILES}
                Job/Job.cc Job/TcpHandlerJob.cc 
                Job/HttpClientJob.cc Job/HttpHandlerJob.cc 
                Job/SilentPushJob.cc Job/WeComUploadJob.cc
                Job/MessageCallbackJob.cc Job/GetChatInfoJob.cc)
SET(COMP_FILES  ${COMP_FILES}
                Codec/HttpCodec.cc)
SET(COMP_FILES  ${COMP_FILES}
                WeCom/ServerMessage.cc 
                WeCom/TextServerMessage.cc WeCom/MarkdownServerMessage.cc 
                WeCom/ImageServerMessage.cc WeCom/NewsServerMessage.cc 
                WeCom/MiniProgramServerMessage.cc WeCom/FileServerMessage.cc 
                WeCom/ClientMessage.cc
                WeCom/TextClientMessage.cc WeCom/ImageClientMessage.cc
                WeCom/EventClientMessage.cc WeCom/AttachmentClientMessage.cc
                WeCom/MixedClientMessage.cc WeCom/ClientMessageImpl.cc)
SET(COMP_FILES  ${COMP_FILES}
                Utility/Common.cc Utility/Logger.cc Utility/HttpPackage.cc
                Utility/MemoryBuffer.cc Utility/CronTrigger.cc Utility/SyncFileLogger.cc)
SET(COMP_FILES  ${COMP_FILES}
                ThirdParty/tinyxml2/tinyxml2.cpp
                ThirdParty/WXBizMsgCrypt/WXBizMsgCrypt.cpp)

SET(INSTALL_HEADERS_CORE    Core/Engine.h)
SET(INSTALL_HEADERS_CODEC   Codec/Codec.h)
SET(INSTALL_HEADERS_UTILITY Utility/Common.h Utility/CronTrigger.h Utility/HttpPackage.h
                            Utility/Logger.h Utility/MemoryBuffer.h)
SET(INSTALL_HEADERS_JOB     Job/Job.h Job/HttpClientJob.h Job/SilentPushJob.h
                            Job/WeComUploadJob.h Job/MessageCallbackJob.h Job/GetChatInfoJob.h)
SET(INSTALL_HEADERS_WECOM   WeCom/ServerMessage.h WeCom/ClientMessage.h
                            WeCom/TextServerMessage.h WeCom/MarkdownServerMessage.h
                            WeCom/ImageServerMessage.h WeCom/NewsServerMessage.h
                            WeCom/FileServerMessage.h WeCom/MiniProgramServerMessage.h
                            WeCom/TextClientMessage.h WeCom/ImageClientMessage.h
                            WeCom/EventClientMessage.h WeCom/AttachmentClientMessage.h
                            WeCom/MixedClientMessage.h)

ADD_LIBRARY(wcbot STATIC ${COMP_FILES})

# INSTALL(DIRECTORY ./ DESTINATION include/wcbot FILES_MATCHING PATTERN "*.h")
INSTALL(FILES ${INSTALL_HEADERS_CORE} DESTINATION include/wcbot/Core)
INSTALL(FILES ${INSTALL_HEADERS_CODEC} DESTINATION include/wcbot/Codec)
INSTALL(FILES ${INSTALL_HEADERS_JOB} DESTINATION include/wcbot/Job)
INSTALL(FILES ${INSTALL_HEADERS_WECOM} DESTINATION include/wcbot/WeCom)
INSTALL(FILES ${INSTALL_HEADERS_UTILITY} DESTINATION include/wcbot/Utility)

INSTALL(TARGETS wcbot ARCHIVE DESTINATION lib)
