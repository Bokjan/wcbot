#include "WeComUploadJob.h"

#include <rapidjson/document.h>

#include "../Core/Engine.h"
#include "../Core/EngineImpl.h"
#include "../Core/WorkerThread.h"
#include "../Job/HttpClientJob.h"
#include "../Utility/Logger.h"

namespace wcbot {

WeComUploadJob::WeComUploadJob()
    : Job(), Data(nullptr), Length(0), Code(0), State(StateEnum::kUploadMediaReq) {}

#define BOUNDARY_STR "----wcbotboundary260817----"

Job::Step WeComUploadJob::OnStep(Job *Trigger) {
  switch (State) {
    case StateEnum::kUploadMediaReq: {
      constexpr int kTimeoutMS = 10000;
      constexpr uint64_t kMaxSize = 20 * 1024 * 1024;
      if (Length > kMaxSize) {
        ErrCode = kErrTooLarge;
        return Step::kDone;
      }
      auto *J = new HttpClientJob();
      auto &Request = J->Request;
      Request.SetUrl(Engine::Get().GetImpl().Config.Bot.WebHookUploadMedia);
      Request.Method = HttpRequest::MethodEnum::kPost;
      Request.Headers.insert(std::make_pair("Content-Type",
                                            "multipart/form-data; "
                                            "boundary=" BOUNDARY_STR));
      std::string &Body = Request.Body;
      Body.append("--" BOUNDARY_STR "\r\n");
      Body.append("Content-Disposition: form-data; name=\"media\"; filename=\"");
      Body.append(FileName);
      Body.push_back('"');
      Body.append("\r\nContent-Type: application/octet-stream\r\n\r\n");
      Body.append(reinterpret_cast<const char *>(Data), Length);
      Body.append("\r\n--" BOUNDARY_STR "--\r\n");
      J->TimeoutMS = kTimeoutMS;
      InvokeChild(J);
      State = StateEnum::kUploadMediaRsp;
      return Step::kWaiting;
    }

    case StateEnum::kUploadMediaRsp: {
      auto *Rsp = AsJob<HttpClientJob>(Trigger);
      do {
        if (Rsp == nullptr) {
          break;
        }
        if (Rsp->ErrCode == kErrTimeout) {
          LOG_WARN("%s", "WeComUploadJob timeout!");
          ErrCode = kErrTimeout;
          break;
        }
        auto &Response = Rsp->Response;
        if (Response.StatusCode == 0 || Response.Body.empty()) {
          LOG_ERROR("WeComUploadJob, StatusCode=%d, BodyLen=%u, abnormal!", Response.StatusCode,
                    Response.Body.length());
          ErrCode = kErrRspFailed;
          break;
        }
        rapidjson::Document Json;
        Json.Parse(Response.Body.c_str());
        if (!Json.IsObject() || !Json.HasMember("errcode") || !Json.HasMember("errmsg") ||
            !Json["errcode"].IsInt() || !Json["errmsg"].IsString()) {
          LOG_WARN("WeComUploadJob rsp invalid, body: %s", Response.Body.c_str());
          ErrCode = kErrRspPkgInvalid;
          break;
        }
        if (Json["errcode"].GetInt() != 0) {
          LOG_WARN("WeComUploadJob errcode=%d errmsg=%s", Json["errcode"].GetInt(),
                   Json["errmsg"].GetString());
          Code = Json["errcode"].GetInt();
          Msg = Json["errmsg"].GetString();
          break;
        }
        if (!Json.HasMember("media_id") || !Json["media_id"].IsString()) {
          ErrCode = kErrRspNoMediaId;
          break;
        }
        MediaId = Json["media_id"].GetString();
      } while (false);
      return Step::kDone;
    }
  }
  return Step::kDone;
}

}  // namespace wcbot