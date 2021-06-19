#pragma once

#include "ServerMessage.h"

#include <cstdint>

namespace wcbot {
namespace wecom {

class MarkdownServerMessage final : public ServerMessage {
 public:
  struct Action {
    std::string Name;           // max 64
    std::string Text;           // max 128
    std::string Value;          // max 128
    std::string ReplaceText;    // max 128
    uint32_t TextColor;         // HEX RGB
    uint32_t BorderColor;       // HEX RGB
    Action() : TextColor(0x66CCFF), BorderColor(0x66CCFF) {}
  };
  bool AtShortName;
  std::string PostId;
  std::string Content;          // max 4096
  std::string CallbackId;
  std::vector<Action> Actions;  // max 20
  std::vector<std::string> VisibleToUser;
  MarkdownServerMessage(): AtShortName(false) { }
  MemoryBuffer* GetXml() const override;
  std::string GetJson() const override;
  bool ValidateFields() const override;
};

}  // namespace wecom
}  // namespace wcbot
