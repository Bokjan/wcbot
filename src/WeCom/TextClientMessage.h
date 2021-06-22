#pragma once

#include "../WeCom/ClientMessage.h"

namespace wcbot {
namespace wecom {

class TextClientMessage : public ClientMessage {
 public:
  explicit TextClientMessage(tinyxml2::XMLElement *Root);
  std::string Content;
};

}  // namespace wecom
}  // namespace wcbot