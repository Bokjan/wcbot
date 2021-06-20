#include "WeComUploadJob.h"

#include <rapidjson/document.h>

#include "../Core/Engine.h"
#include "../Core/EngineImpl.h"
#include "../Core/WorkerThread.h"
#include "../Utility/Logger.h"
#include "HttpClientJob.h"

namespace wcbot {

WeComUploadJob::WeComUploadJob(Job *Receiver)
    : Job(Receiver), Code(0), State(StateEnum::kUploadMediaReq) {}

void WeComUploadJob::Do(Job *Trigger) {
  switch (State) {
    case StateEnum::kUploadMediaReq:
      this->DoUploadMediaReq();
      break;
    case StateEnum::kUploadMediaRsp:
      this->DoUploadMediaRsp(Trigger);
      break;
    case StateEnum::kError:
      this->DoError();
      break;
    default:
      break;
  }
}

void WeComUploadJob::OnTimeout(Job *Trigger) {
  SafeParent()->OnTimeout(this);
  DeleteThis();
}

void WeComUploadJob::DoError() {
  SafeParent()->Do(this);
  DeleteThis();
}

#define BOUNDARY_STR "----wcbotboundary260817----"

void WeComUploadJob::DoUploadMediaReq() {
  constexpr int kTimeoutMS = 10000;
  constexpr uint64_t kMaxSize = 20 * 1024 * 1024;
  if (Length > kMaxSize) {
    this->ErrCode = kErrTooLarge;
    this->Do();
    return;
  }
  auto J = new HttpClientJob(this);
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
  J->Do();
  State = StateEnum::kUploadMediaRsp;
}

void WeComUploadJob::DoUploadMediaRsp(Job *RspBase) {
  // check
  auto Rsp = dynamic_cast<HttpClientJob *>(RspBase);
  LOG_DEBUG("%s", Rsp->Response.Body.c_str());
  do {
    auto &Response = Rsp->Response;
    if (Response.StatusCode == 0 || Response.Body.empty()) {
      LOG_ERROR("%s", "WeComUploadJob, StatusCode=%d, BodyLen=%u, abnormal!", Response.StatusCode,
                Response.Body.length());
      ErrCode = kRspFailed;
      break;
    }
    rapidjson::Document Json;
    Json.Parse(Response.Body.c_str());
    if (!Json.IsObject() || !Json.HasMember("errcode") || !Json.HasMember("errmsg") ||
        !Json["errcode"].IsInt() || !Json["errmsg"].IsString()) {
      LOG_WARN("WeComUploadJob rsp invalid, body: %s", Response.Body.c_str());
      ErrCode = kRspPkgInvalid;
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
      ErrCode = kRspNoMediaId;
      break;
    }
    MediaId = Json["media_id"].GetString();
  } while (false);
  SafeParent()->Do(this);
  DeleteThis();
}

}  // namespace wcbot
