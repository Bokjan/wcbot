#pragma once

#include <string>

namespace tinyxml2 {
class XMLElement;
}

namespace wcbot {
namespace wecom {

class ClientMessage {
 public:
  ClientMessage(void) = delete;
  virtual ~ClientMessage() = 0;
  bool ParseCommon(tinyxml2::XMLElement *Root);

  enum class ChatTypeEnum : int { kUnknown, kSingle, kGroup };

  ChatTypeEnum ChatType;
  std::string MsgId;
  std::string ChatId;
  std::string WebHookUrl;
  std::string GetChatInfoUrl;
  struct {
    std::string UserId;
    std::string Name;
    std::string Alias;
  } From;
  bool HasExtractError;
};

}  // namespace wecom
}  // namespace wcbot
