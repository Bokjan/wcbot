#pragma once

#include "ServerMessage.h"

namespace wcbot {
namespace wecom {

class ImageServerMessage final : public ServerMessage {
 public:
  std::string ImageData;
  std::vector<std::string> VisibleToUser;
  std::string GetJson() const override;
  bool ValidateFields() const override;
};

}  // namespace wecom
}  // namespace wcbot
