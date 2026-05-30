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

// GetChatInfoJob — fetch WeCom chat info from a given URL and (optionally)
// invoke a user-supplied callback once the result is ready.

class GetChatInfoJob final : public Job {
 public:
  explicit GetChatInfoJob(const std::string &Url);
  GetChatInfoJob(const GetChatInfoJob &) = delete;
  GetChatInfoJob(const GetChatInfoJob &&) = delete;

  Step OnStep(Job *Trigger) override;

  enum ErrEnum { kErrHttp = 1, kErrJson };

  ChatInfo Response;

  using FnCallback = std::function<void(int, ChatInfo &)>;
  FnCallback FinishCallback;

 private:
  enum class StateEnum { kSendReq, kSendRsp };
  StateEnum State;
  const std::string &GetChatInfoUrl;
};

}  // namespace wcbot