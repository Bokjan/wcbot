#include "TextServerMessage.h"

#include <cinttypes>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../Utility/Logger.h"
#include "../Utility/MemoryBuffer.h"

namespace wcbot {
namespace wecom {

bool TextServerMessage::ValidateFields() const {
  constexpr auto kContentMaxLength = 2048UL;
  if (Content.length() > kContentMaxLength || Content.empty()) {
    LOG_WARN("`TextServerMessage::Content` length=%" PRIu64 ", max=%" PRIu64 ", validate failed!",
             Content.length(), kContentMaxLength);
    return false;
  }
  return true;
}

void TextServerMessage::GetXml(MemoryBuffer *Output) const {
  MEMBUF_APP(Output, "<xml>");
  MEMBUF_APP(Output, "<MsgType>text</MsgType>");
  // visible_to_user
  if (!VisibleToUser.empty()) {
    MEMBUF_APP(Output, "<VisibleToUser>");
    thread_local std::string Buffer;
    Buffer.clear();
    for (const auto &Item : VisibleToUser) {
      Buffer.append(Item);
      Buffer.push_back('|');
    }
    Buffer.pop_back();  // omit last |
    Output->Append(Buffer);
    MEMBUF_APP(Output, "</VisibleToUser>");
  }
  MEMBUF_APP(Output, "<Text>");
  // content
  MEMBUF_APP(Output, "<Content><![CDATA[");
  Output->Append(Content);
  MEMBUF_APP(Output, "]]></Content>");
  // mentioned list
  if (!MentionedList.empty()) {
    MEMBUF_APP(Output, "<MentionedList>");
    for (const auto &Item : MentionedList) {
      MEMBUF_APP(Output, "<Item><![CDATA[");
      Output->Append(Item);
      MEMBUF_APP(Output, "]]></Item>");
    }
    MEMBUF_APP(Output, "</MentionedList>");
  }
  // mentioned mobile list
  if (!MentionedMobileList.empty()) {
    MEMBUF_APP(Output, "<MentionedMobileList>");
    for (const auto &Item : MentionedList) {
      MEMBUF_APP(Output, "<Item><![CDATA[");
      Output->Append(Item);
      MEMBUF_APP(Output, "]]></Item>");
    }
    MEMBUF_APP(Output, "</MentionedMobileList>");
  }
  MEMBUF_APP(Output, "</Text>");
  MEMBUF_APP(Output, "</xml>");
}

std::string TextServerMessage::GetJson() const {
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
    Buffer.pop_back();  // omit last |
    Writer.String(Buffer.c_str(), Buffer.length());
  }
  // post_id
  if (!PostId.empty()) {
    Writer.Key("post_id");
    Writer.String(PostId.c_str(), PostId.length());
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
    Buffer.pop_back();  // omit last |
    Writer.String(Buffer.c_str(), Buffer.length());
  }
  // msgtype
  Writer.Key("msgtype");
  Writer.String("text");
  // text
  Writer.Key("text");
  Writer.StartObject();
  Writer.Key("content");
  Writer.String(Content.c_str(), Content.length());
  if (!MentionedList.empty()) {
    Writer.Key("mentioned_list");
    Writer.StartArray();
    for (const auto &Item : MentionedList) {
      Writer.String(Item.c_str(), Item.length());
    }
    Writer.EndArray();
  }
  if (!MentionedMobileList.empty()) {
    Writer.Key("mentioned_mobile_list");
    Writer.StartArray();
    for (const auto &Item : MentionedMobileList) {
      Writer.String(Item.c_str(), Item.length());
    }
    Writer.EndArray();
  }
  Writer.EndObject();
  // end
  Writer.EndObject();
  return std::string(SB.GetString(), SB.GetSize());
}

}  // namespace wecom
}  // namespace wcbot
