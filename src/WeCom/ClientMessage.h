#pragma once

#include <string>

namespace tinyxml2 {
class XMLElement;
}

namespace wcbot {
namespace wecom {

class ClientMessage {
 public:
  explicit ClientMessage(tinyxml2::XMLElement *Root);
  virtual ~ClientMessage() = default;
  void ParseCommon(tinyxml2::XMLElement *Root);

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

#define CLIENT_MESSAGE_DERIVED_GUARD(o) \
  do {                                  \
    if ((o).HasExtractError) {              \
      return;                           \
    } else {                            \
      (o).HasExtractError = true;           \
    }                                   \
  } while (false)

}  // namespace wecom
}  // namespace wcbot
