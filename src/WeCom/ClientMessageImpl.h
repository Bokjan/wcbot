#pragma once

#include <memory>

#include "../WeCom/ClientMessage.h"

namespace wcbot {
namespace wecom {
namespace client_message_impl {

// Returns a constructed ClientMessage on success, or nullptr on failure (XML
// parse / unknown MsgType / per-message extract failure). Ownership is
// transferred to the caller.
std::unique_ptr<ClientMessage> GenerateClientMessageByXml(const std::string& Xml);

}  // namespace client_message_impl
}  // namespace wecom
}  // namespace wcbot