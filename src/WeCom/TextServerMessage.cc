#include "TextServerMessage.h"

#include <cinttypes>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../Core/MemoryBuffer.h"
#include "../Utility/Logger.h"

namespace wcbot {
namespace wecom {

bool TextServerMessage::ValidateFields() {
  constexpr auto kContentMaxLength = 2048UL;
  if (Content.length() > kContentMaxLength || Content.empty()) {
    LOG_WARN("`TextServerMessage::Content` length=%" PRIu64 ", max=%" PRIu64 ", validate failed!",
             Content.length(), kContentMaxLength);
    return false;
  }
  return true;
}

MemoryBuffer* TextServerMessage::GetXml() {
  // todo
  return nullptr;
}

MemoryBuffer* TextServerMessage::GetJson() {
  // start
  rapidjson::StringBuffer SB;
  rapidjson::Writer<rapidjson::StringBuffer> Writer(SB);
  Writer.StartObject();
  // chatid
  if (!ChatId.empty()) {
    Writer.Key("chatid");
    thread_local std::string Buffer;
    Buffer.clear();
    for (const auto &Item : ChatId) {
      Buffer.append(Item);
      Buffer.push_back('|');
    }
    Buffer.pop_back(); // omit last |
    Writer.String(Buffer.c_str(), Buffer.length(), false);
  }
  // post_id
  if (!PostId.empty()) {
    Writer.Key("post_id");
    Writer.String(PostId.c_str(), PostId.length(), false);
  }
  // visible_to_user
  if (!VisibleToUser.empty()) {
    Writer.Key("visible_to_user");
    thread_local std::string Buffer;
    Buffer.clear();
    for (const auto &Item : VisibleToUser) {
      Buffer.append(Item);
      Buffer.push_back('|');
    }
    Buffer.pop_back(); // omit last |
    Writer.String(Buffer.c_str(), Buffer.length(), false);
  }
  // msgtype
  Writer.Key("msgtype");
  Writer.String("text");
  Writer.Key("text");
  Writer.StartObject();
  // content
  Writer.Key("content");
  Writer.String(Content.c_str(), Content.length(), false);
  // mentioned list
  if (!MentionedList.empty()) {
    Writer.Key("mentioned_list");
    Writer.StartArray();
    for (const auto &Item : MentionedList) {
      Writer.String(Item.c_str(), Item.length(), false);
    }
    Writer.EndArray();
  }
  // mentioned mobile list
  if (!MentionedMobileList.empty()) {
    Writer.Key("mentioned_mobile_list");
    Writer.StartArray();
    for (const auto &Item : MentionedMobileList) {
      Writer.String(Item.c_str(), Item.length(), false);
    }
    Writer.EndArray();
  }
  // end
  Writer.EndObject();
  Writer.EndObject();
  // build MemoryBuffer
  MemoryBuffer* MB = MemoryBuffer::Create();
  MB->Append(SB.GetString(), SB.GetSize());
  return MB;
}

}  // namespace wecom
}  // namespace wcbot
