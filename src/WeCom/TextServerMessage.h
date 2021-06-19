#pragma once

#include "ServerMessage.h"

namespace wcbot {
namespace wecom {

class TextServerMessage final : public ServerMessage {
public:
  std::string PostId;
  std::string Content;  // max 2048
  std::vector<std::string> MentionedList;
  std::vector<std::string> MentionedMobileList;
  std::vector<std::string> VisibleToUser;
  MemoryBuffer* GetXml() const override;
  std::string GetJson() const override;
  bool ValidateFields() const override;
};

}
}
