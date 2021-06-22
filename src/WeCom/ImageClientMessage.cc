#include "ImageClientMessage.h"

#include "../ThirdParty/tinyxml2/tinyxml2.h"

namespace wcbot {
namespace wecom {

ImageClientMessage::ImageClientMessage(tinyxml2::XMLElement *Root) : ClientMessage(Root) {
  CLIENT_MESSAGE_DERIVED_GUARD(*this);
  do {
    auto Find = Root->FirstChildElement("Image");
    if (Find == nullptr) {
      break;
    }
    Find = Find->FirstChildElement("ImageUrl");
    if (Find == nullptr) {
      break;
    }
    ImageUrl = Find->GetText();
    HasExtractError = false;
  } while (false);
}

}  // namespace wecom
}  // namespace wcbot
