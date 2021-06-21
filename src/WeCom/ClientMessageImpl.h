#pragma once

#include "../WeCom/ClientMessage.h"

namespace wcbot {
namespace wecom {
namespace client_message_impl {

ClientMessage* GenerateClientMessageByXml(const std::string& Xml);

}  // namespace client_message_impl
}  // namespace wecom
}  // namespace wcbot