#pragma once

#include "../WeCom/ClientMessage.h"

namespace wcbot {
namespace wecom {

class EventClientMessage : public ClientMessage {
 public:
  explicit EventClientMessage(tinyxml2::XMLElement *Root);
  enum class EventEnum : int {
    kAddToChat,       // 添加进会话
    kDeleteFromChat,  // 被移出会话
    kEnterChat        // 进入单聊
  };
  EventEnum Event;
  std::string AppVersion;
};

}  // namespace wecom
}  // namespace wcbot