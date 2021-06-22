#include "ClientMessage.h"

#include <cstring>

#include "../ThirdParty/tinyxml2/tinyxml2.h"

namespace wcbot {
namespace wecom {

ClientMessage::ClientMessage(tinyxml2::XMLElement *Root) { ParseCommon(Root); }

void ClientMessage::ParseCommon(tinyxml2::XMLElement *Root) {
  HasExtractError = true;
  do {
    // ChatId
    auto Find = Root->FirstChildElement("ChatId");
    if (Find == nullptr) {
      break;
    }
    ChatId = Find->GetText();
    // GetChatInfoUrl
    Find = Root->FirstChildElement("GetChatInfoUrl");
    if (Find != nullptr) {
      GetChatInfoUrl = Find->GetText();
    }
    // MsgId
    Find = Root->FirstChildElement("MsgId");
    if (Find == nullptr) {
      break;
    }
    MsgId = Find->GetText();
    // ChatType
    Find = Root->FirstChildElement("ChatType");
    if (Find == nullptr) {
      break;
    }
    if (strcmp(Find->GetText(), "single") == 0) {
      ChatType = ChatTypeEnum::kSingle;
    } else if (strcmp(Find->GetText(), "group") == 0) {
      ChatType = ChatTypeEnum::kGroup;
    } else {
      break;
    }
    // MsgType
    Find = Root->FirstChildElement("MsgType");
    if (Find == nullptr) {
      break;
    }
    // From
    Find = Root->FirstChildElement("From");
    auto FromFind = Find->FirstChildElement("UserId");
    if (FromFind == nullptr) {
      break;
    }
    From.UserId = FromFind->GetText();
    FromFind = Find->FirstChildElement("Name");
    if (FromFind == nullptr) {
      break;
    }
    From.Name = FromFind->GetText();
    FromFind = Find->FirstChildElement("Alias");
    if (FromFind == nullptr) {
      break;
    }
    From.Alias = FromFind->GetText();
    HasExtractError = false;
  } while (false);
}

}  // namespace wecom
}  // namespace wcbot
