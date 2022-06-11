#include "GetChatInfoJob.h"

#include <algorithm>
#include <functional>

#include <rapidjson/document.h>

#include "../Job/HttpClientJob.h"
#include "../Utility/Logger.h"

#define BREAK_ON_FALSE(x) \
  if (!(x)) {             \
    break;                \
  }

namespace wcbot {

static bool ExtractInt(rapidjson::Value &Doc, const char *Name, int &Dst) {
  if (!Doc.HasMember(Name) || !Doc[Name].IsInt()) {
    return false;
  }
  Dst = Doc[Name].GetInt();
  return true;
}

static bool ExtractString(rapidjson::Value &Doc, const char *Name, std::string &Dst) {
  if (!Doc.HasMember(Name) || !Doc[Name].IsString()) {
    return false;
  }
  Dst.assign(Doc[Name].GetString(), Doc[Name].GetStringLength());
  return true;
}

GetChatInfoJob::GetChatInfoJob(const std::string &Url)
    : Job(),
      FinishCallback([](int, ChatInfo &) {}),
      State(StateEnum::kSendReq),
      GetChatInfoUrl(Url) {}

void GetChatInfoJob::Do(Job *Trigger) {
  LOG_DEBUG("GetChatInfoJob::Do Trigger=%p, State=%d", Trigger, State);
  switch (State) {
    case StateEnum::kSendReq:
      this->DoSendReq();
      break;
    case StateEnum::kSendRsp:
      this->DoSendRsp(Trigger);
      break;
    case StateEnum::kFinish:
      this->DoFinish();
      break;
    default:
      break;
  }
}

void GetChatInfoJob::DoSendReq() {
  constexpr int kTimeoutMS = 4000;
  auto J = new HttpClientJob();
  J->Request.SetUrl(GetChatInfoUrl);
  J->Request.Method = HttpRequest::MethodEnum::kGet;
  J->TimeoutMS = kTimeoutMS;
  InvokeChild(J);
  State = StateEnum::kSendRsp;
}

void GetChatInfoJob::DoSendRsp(Job *RspBase) {
  // check
  do {
    if (ErrCode == kErrTimeout) {
      LOG_WARN("GetChatInfoJob timeout");
      break;
    }
    auto &HttpRsp = dynamic_cast<HttpClientJob *>(RspBase)->Response;
    if (HttpRsp.StatusCode == 0 || HttpRsp.Body.empty()) {
      LOG_ERROR("GetChatInfoJob, StatusCode=%d, BodyLen=%u, abnormal!", HttpRsp.StatusCode,
                HttpRsp.Body.length());
      ErrCode = kErrHttp;
      break;
    }
    rapidjson::Document Json;
    Json.Parse(HttpRsp.Body.c_str());
    if (Json.HasParseError()) {
      LOG_ERROR("GetChatInfoJob parse rsp json failed, rapidjson errcode: %d, body: %s",
                Json.GetParseError(), HttpRsp.Body.c_str());
      ErrCode = kErrJson;
      break;
    }

    // ExtractJsonFields
    auto JsonExtractInt =
        std::bind(ExtractInt, std::ref(Json), std::placeholders::_1, std::placeholders::_2);
    auto JsonExtractStr =
        std::bind(ExtractString, std::ref(Json), std::placeholders::_1, std::placeholders::_2);
    ErrCode = kErrJson;
    do {
      std::string Tmp;
      BREAK_ON_FALSE(JsonExtractInt("errcode", Response.ErrCode));
      BREAK_ON_FALSE(JsonExtractStr("errmsg", Response.ErrMsg));
      BREAK_ON_FALSE(JsonExtractStr("chatid", Response.ChatId));
      BREAK_ON_FALSE(JsonExtractStr("name", Response.Name));
      BREAK_ON_FALSE(JsonExtractStr("chattype", Tmp));
      using ChatTypePair = std::pair<const char *, ChatInfo::ChatTypeEnum>;
      static const ChatTypePair ChatTypeMapping[] = {
          {"single", ChatInfo::ChatTypeEnum::kSingle},
          {"group", ChatInfo::ChatTypeEnum::kGroup},
          {"blackboard", ChatInfo::ChatTypeEnum::kBlackboard},
          {"blackboard_reply", ChatInfo::ChatTypeEnum::kBlackboardReply},
      };
      auto PairIt = std::find_if(
          std::begin(ChatTypeMapping), std::end(ChatTypeMapping),
          [&Tmp](const ChatTypePair &Pair) { return strcmp(Pair.first, Tmp.c_str()) == 0; });
      if (PairIt != std::end(ChatTypeMapping)) {
        Response.ChatType = PairIt->second;
      } else {
        LOG_ERROR("unknown chattype: %s", Tmp.c_str());
      }
      if (Json.HasMember("members") && Json["members"].IsArray()) {
        for (auto &Member : Json["members"].GetArray()) {
          if (!Member.IsObject()) {
            continue;
          }
          auto MemberExtractStr = std::bind(ExtractString, std::ref(Member), std::placeholders::_1,
                                            std::placeholders::_2);
          ChatInfo::Member TmpMember;
          MemberExtractStr("userid", TmpMember.UserId);
          MemberExtractStr("alias", TmpMember.Alias);
          MemberExtractStr("name", TmpMember.Name);
          Response.Members.emplace_back(TmpMember);
        }
      }
    } while (false);
    ErrCode = 0;
  } while (false);
  State = StateEnum::kFinish;
  this->Do();
}

void GetChatInfoJob::DoFinish() {
  FinishCallback(ErrCode, Response);
  NotifyParent();
  DeleteThis();
}

}  // namespace wcbot
