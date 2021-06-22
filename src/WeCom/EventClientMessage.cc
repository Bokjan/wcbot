#include "EventClientMessage.h"

#include <cstring>

#include "../ThirdParty/tinyxml2/tinyxml2.h"

namespace wcbot {
namespace wecom {

#define STRNCMP_CCPTR_LITERAL(p, l) (strncmp(p, l, sizeof(l) - 1) == 0)

EventClientMessage::EventClientMessage(tinyxml2::XMLElement *Root) : ClientMessage(Root) {
  CLIENT_MESSAGE_DERIVED_GUARD(*this);
  do {
    auto Find = Root->FirstChildElement("Event");
    if (Find == nullptr) {
      break;
    }
    Find = Find->FirstChildElement("EventType");
    if (Find == nullptr) {
      break;
    }
    if (STRNCMP_CCPTR_LITERAL(Find->GetText(), "add_to_chat")) {
      Event = EventEnum::kAddToChat;
    } else if (STRNCMP_CCPTR_LITERAL(Find->GetText(), "delete_from_chat")) {
      Event = EventEnum::kDeleteFromChat;
    } else if (STRNCMP_CCPTR_LITERAL(Find->GetText(), "enter_chat")) {
      Event = EventEnum::kEnterChat;
    } else {
      break;
    }
    Find = Root->FirstChildElement("AppVersion");
    if (Find == nullptr) {
      break;
    }
    AppVersion = Find->GetText();
    HasExtractError = false;
  } while (false);
}

}  // namespace wecom
}  // namespace wcbot
