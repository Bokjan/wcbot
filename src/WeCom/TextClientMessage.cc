#include "TextClientMessage.h"

#include "../ThirdParty/tinyxml2/tinyxml2.h"

namespace wcbot {
namespace wecom {

TextClientMessage::TextClientMessage(tinyxml2::XMLElement *Root) : ClientMessage(Root) {
  CLIENT_MESSAGE_DERIVED_GUARD(*this);
  do {
    auto Find = Root->FirstChildElement("Text");
    if (Find == nullptr) {
      break;
    }
    Find = Find->FirstChildElement("Content");
    if (Find == nullptr) {
      break;
    }
    Content = Find->GetText();
    HasExtractError = false;
  } while (false);
}

}  // namespace wecom
}  // namespace wcbot
