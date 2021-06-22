#pragma once

#include "../WeCom/ClientMessage.h"

namespace wcbot {
namespace wecom {

class AttachmentClientMessage : public ClientMessage {
 public:
  explicit AttachmentClientMessage(tinyxml2::XMLElement *Root);
  std::string CallbackId;
  struct {
    std::string Name;
    std::string Value;
    // type is always button
  } Action;
};

}  // namespace wecom
}  // namespace wcbot
