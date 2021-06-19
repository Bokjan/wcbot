#include "SilentPushJob.h"

#include <rapidjson/document.h>

#include "../Core/Engine.h"
#include "../Core/EngineImpl.h"
#include "../Utility/Logger.h"
#include "../WeCom/ServerMessage.h"
#include "HttpClientJob.h"

namespace wcbot {

void SilentPushJob::Do(Job *Trigger) {
  switch (State) {
    case StateEnum::kSendReq:
      this->DoSendReq();
      break;
    case StateEnum::kSendRsp:
      this->DoSendRsp();
      break;
    default:
      break;
  }
}

void SilentPushJob::OnTimeout(Job *Trigger) {
  LOG_WARN("SilentPushJob::OnTimeout");
  DeleteThis();
}

void SilentPushJob::DoSendReq() {
  constexpr int kTimeoutMS = 5000;
  SendJob = new HttpClientJob(this);
  SendJob->Request.SetUrl(Engine::Get().GetImpl()->Config.Bot.WebHookSend);
  SendJob->Request.Headers.insert(std::make_pair("Content-Type", "application/json"));
  SendJob->Request.Body = std::move(Message->GetJson());
  SendJob->Request.Method = HttpRequest::MethodEnum::kPost;
  SendJob->TimeoutMS = kTimeoutMS;
  SendJob->Do();
  State = StateEnum::kSendRsp;
}

void SilentPushJob::DoSendRsp() {
  // check
  do {
    auto &Response = SendJob->Response;
    if (Response.StatusCode == 0 || Response.Body.empty()) {
      LOG_ERROR("%s", "SilentPushJob, StatusCode=%d, BodyLen=%u, abnormal!", Response.StatusCode,
                Response.Body.length());
      break;
    }
    rapidjson::Document Json;
    Json.Parse(Response.Body.c_str());
    if (!Json.IsObject() || !Json.HasMember("errcode") || !Json.HasMember("errmsg") ||
        !Json["errcode"].IsInt() || !Json["errmsg"].IsString()) {
      LOG_WARN("SilentPushJob rsp invalid, body: %s", Response.Body.c_str());
      break;
    }
    if (Json["errcode"].GetInt() != 0) {
      LOG_WARN("SilentPushJob errcode=%d errmsg=%s", Json["errcode"].GetInt(),
               Json["errmsg"].GetString());
    }
  } while (false);
  // a silent job
  // don't callback parent
  DeleteThis();
}

}  // namespace wcbot
