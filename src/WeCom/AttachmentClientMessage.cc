#include "AttachmentClientMessage.h"

#include "../ThirdParty/tinyxml2/tinyxml2.h"

namespace wcbot {
namespace wecom {

AttachmentClientMessage::AttachmentClientMessage(tinyxml2::XMLElement *Root) : ClientMessage(Root) {
  CLIENT_MESSAGE_DERIVED_GUARD(*this);
  do {
    auto Find = Root->FirstChildElement("Attachment");
    if (Find == nullptr) {
      break;
    }
    Find = Find->FirstChildElement("CallbackId");
    if (Find == nullptr) {
      break;
    }
    CallbackId = Find->GetText();
    Find = Find->FirstChildElement("Actions");
    if (Find == nullptr) {
      break;
    }
    auto Second = Find->FirstChildElement("Name");
    if (Second == nullptr) {
      break;
    }
    Action.Name = Second->GetText();
    Second = Find->FirstChildElement("Value");
    if (Second == nullptr) {
      break;
    }
    Action.Value = Second->GetText();
    HasExtractError = false;
  } while (false);
}

}  // namespace wecom
}  // namespace wcbot
