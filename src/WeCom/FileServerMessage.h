#pragma once

#include "ServerMessage.h"

namespace wcbot {
namespace wecom {

class FileServerMessage final : public ServerMessage {
 public:
  std::string MediaId;
  std::string GetJson() const override;
  bool ValidateFields() const override;
};

}  // namespace wecom
}  // namespace wcbot
