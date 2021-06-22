#include "MixedClientMessage.h"

#include <cstring>

#include "../ThirdParty/tinyxml2/tinyxml2.h"

namespace wcbot {
namespace wecom {

#define STRNCMP_CCPTR_LITERAL(p, l) (strncmp(p, l, sizeof(l) - 1) == 0)

MixedClientMessage::MixedClientMessage(tinyxml2::XMLElement *Root) : ClientMessage(Root) {
  CLIENT_MESSAGE_DERIVED_GUARD(*this);
  do {
    auto Find = Root->FirstChildElement("MixedMessage");
    if (Find == nullptr) {
      break;
    }
    for (Find = Find->FirstChildElement("MsgItem"); Find != nullptr;
         Find = Find->NextSiblingElement()) {
      auto MsgType = Find->FirstChildElement("MsgType");
      if (MsgType == nullptr) {
        return;
      }
      if (STRNCMP_CCPTR_LITERAL(MsgType->GetText(), "text")) {
        auto Text = Find->FirstChildElement("Text");
        if (Text == nullptr) {
          return;
        }
        Text = Text->FirstChildElement("Content");
        if (Text == nullptr) {
          return;
        }
        Content = Text->GetText();
      } else if (STRNCMP_CCPTR_LITERAL(MsgType->GetText(), "image")) {
        auto Image = Find->FirstChildElement("Image");
        if (Image == nullptr) {
          return;
        }
        Image = Image->FirstChildElement("ImageUrl");
        if (Image == nullptr) {
          return;
        }
        ImageUrl = Image->GetText();
      } else {
        return;
      }
    }
    HasExtractError = false;
  } while (false);
}

}  // namespace wecom
}  // namespace wcbot
