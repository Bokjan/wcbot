#pragma once

#include "ServerMessage.h"

namespace wcbot {
namespace wecom {

class MiniProgramServerMessage final : public ServerMessage {
 public:
  std::string Title;
  std::string MediaId;
  std::string AppId;
  std::string Page;
  std::string GetJson() const override;
  bool ValidateFields() const override;
};

}  // namespace wecom
}  // namespace wcbot
