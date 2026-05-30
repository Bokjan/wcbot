#include "../WeCom/ClientMessageImpl.h"

#include <cstring>
#include <memory>

#include "../ThirdParty/tinyxml2/tinyxml2.h"
#include "../Utility/Logger.h"

#include "AttachmentClientMessage.h"
#include "EventClientMessage.h"
#include "ImageClientMessage.h"
#include "MixedClientMessage.h"
#include "TextClientMessage.h"

#define STRNCMP_CCPTR_LITERAL(p, l) (strncmp(p, l, sizeof(l) - 1) == 0)

namespace wcbot {
namespace wecom {
namespace client_message_impl {

std::unique_ptr<ClientMessage> GenerateClientMessageByXml(const std::string& XmlStr) {
  tinyxml2::XMLDocument Xml;
  std::unique_ptr<ClientMessage> Msg;
  do {
    // get `MsgType`
    int ParseRet = Xml.Parse(XmlStr.c_str(), XmlStr.length());
    if (ParseRet != 0) {
      LOG_ERROR("tinyxml2 parse decrypted body error, ret=%d, decrypted=%s", ParseRet,
                XmlStr.c_str());
      break;
    }
    auto Root = Xml.FirstChildElement("xml");
    if (Root == nullptr) {
      LOG_ERROR("decrypted xml has no <xml> label, data: %s", XmlStr.c_str());
      break;
    }
    auto MsgType = Root->FirstChildElement("MsgType");
    if (MsgType == nullptr) {
      LOG_ERROR("decrypted xml has no <MsgType> label, data: %s", XmlStr.c_str());
      break;
    }
    // dispatch
    if (STRNCMP_CCPTR_LITERAL(MsgType->GetText(), "text")) {
      Msg.reset(new TextClientMessage(Root));
    } else if (STRNCMP_CCPTR_LITERAL(MsgType->GetText(), "image")) {
      Msg.reset(new ImageClientMessage(Root));
    } else if (STRNCMP_CCPTR_LITERAL(MsgType->GetText(), "event")) {
      Msg.reset(new EventClientMessage(Root));
    } else if (STRNCMP_CCPTR_LITERAL(MsgType->GetText(), "attachment")) {
      Msg.reset(new AttachmentClientMessage(Root));
    } else if (STRNCMP_CCPTR_LITERAL(MsgType->GetText(), "mixed")) {
      Msg.reset(new MixedClientMessage(Root));
    }
  } while (false);
  if (Msg != nullptr && Msg->HasExtractError) {
    Msg.reset();
  }
  return Msg;
}

}  // namespace client_message_impl
}  // namespace wecom
}  // namespace wcbot