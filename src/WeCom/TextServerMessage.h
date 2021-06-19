#pragma once

#include "ServerMessage.h"

namespace wcbot {
namespace wecom {

class TextServerMessage final : public ServerMessage {
public:
  std::string PostId;
  std::string Content;
  std::vector<std::string> MentionedList;
  std::vector<std::string> MentionedMobileList;
  std::vector<std::string> VisibleToUser;
  MemoryBuffer* GetXml() override;
  MemoryBuffer* GetJson() override;
  bool ValidateFields() override;
};

}
}
