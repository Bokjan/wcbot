#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../Job/Job.h"

namespace wcbot {

class HttpClientJob;

struct ChatInfo {
  struct Member {
    std::string UserId;
    std::string Alias;
    std::string Name;
  };
  enum class ChatTypeEnum : int { kUnknown, kSingle, kGroup, kBlackboard, kBlackboardReply };
  int ErrCode;
  std::string ErrMsg;
  std::string ChatId;
  std::string Name;
  ChatTypeEnum ChatType;
  std::vector<Member> Members;
  ChatInfo() : ErrCode(0), ChatType(ChatTypeEnum::kUnknown) {}
};

// `GetChatInfoJob` will fetch WeCom chat info from specified URL

class GetChatInfoJob final : public Job {
 public:
  explicit GetChatInfoJob(const std::string &Url);
  GetChatInfoJob(const GetChatInfoJob &) = delete;
  GetChatInfoJob(const GetChatInfoJob &&) = delete;
  void Do(Job *Trigger = nullptr);

  enum ErrEnum { kErrHttp = 1, kErrJson };

  ChatInfo Response;

  using FnCallback = std::function<void(int, ChatInfo &)>;
  FnCallback FinishCallback;

 private:
  enum class StateEnum { kSendReq, kSendRsp, kFinish };
  StateEnum State;
  const std::string &GetChatInfoUrl;

  void DoSendReq();
  void DoSendRsp(Job *Rsp);
  void DoFinish();
};

}  // namespace wcbot
