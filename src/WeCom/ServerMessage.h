#pragma once

#include <string>
#include <vector>

namespace wcbot {

class MemoryBuffer;

namespace wecom {

constexpr auto kChatIdAll = "@all";
constexpr auto kChatIdAllGroup = "@all_group";
constexpr auto kChatIdAllSubscriber = "@all_subscriber";
constexpr auto kChatIdAllBlackboard = "@all_blackboard";

class ServerMessage {
 public:
  virtual ~ServerMessage();
  virtual MemoryBuffer* GetXml();
  virtual MemoryBuffer* GetJson() = 0;
  virtual bool ValidateFields() = 0;

  std::vector<std::string> ChatId;
};

}  // namespace wecom

}  // namespace wcbot
