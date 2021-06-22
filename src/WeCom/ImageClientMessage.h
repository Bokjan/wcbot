#pragma once

#include "../WeCom/ClientMessage.h"

namespace wcbot {
namespace wecom {

class ImageClientMessage : public ClientMessage {
 public:
  explicit ImageClientMessage(tinyxml2::XMLElement *Root);
  std::string ImageUrl;
};

}  // namespace wecom
}  // namespace wcbot
