#pragma once

#include "../WeCom/ClientMessage.h"

namespace wcbot {
namespace wecom {

class MixedClientMessage : public ClientMessage {
 public:
  explicit MixedClientMessage(tinyxml2::XMLElement *Root);
  std::string Content;
  std::string ImageUrl;
};

}  // namespace wecom
}  // namespace wcbot
