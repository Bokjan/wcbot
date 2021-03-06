#include "MarkdownServerMessage.h"

#include <cinttypes>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../Utility/Logger.h"

namespace wcbot {
namespace wecom {

bool MarkdownServerMessage::ValidateFields() const {
  constexpr size_t kMaxContentLength = 4096;
  constexpr size_t kMaxActionCount = 20;
  if (Content.length() > kMaxContentLength) {
    LOG_WARN("MarkdownServerMessage Content.length=%" PRIu64 ", larger than %" PRIu64,
             Content.length(), kMaxContentLength);
    return false;
  }
  if (!VisibleToUser.empty()) {
    if (ChatId.empty()) {
      LOG_WARN("%s", "VisibleToUser set but ChatId is empty");
      // don't break
    }
  }
  if (Actions.size() > kMaxActionCount) {
    LOG_WARN("MarkdownServerMessage Actions.size=%" PRIu64 ", larger than %" PRIu64, Actions.size(),
             kMaxActionCount);
    return false;
  }
  return true;
}

std::string MarkdownServerMessage::GetJson() const {
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
  Writer.String("markdown");
  // markdown
  Writer.Key("markdown");
  Writer.StartObject();
  Writer.Key("content");
  Writer.String(Content.c_str(), Content.length());
  Writer.Key("at_short_name");
  Writer.Bool(AtShortName);
  if (!Actions.empty()) {
    Writer.Key("attachments");
    Writer.StartArray();
    Writer.StartObject();
    Writer.Key("callback_id");
    Writer.String(CallbackId.c_str(), CallbackId.length());
    Writer.Key("actions");
    Writer.StartArray();
    for (const auto &Item : Actions) {
      Writer.StartObject();
      Writer.Key("name");
      Writer.String(Item.Name.c_str(), Item.Name.length());
      Writer.Key("text");
      Writer.String(Item.Text.c_str(), Item.Text.length());
      Writer.Key("type");
      Writer.String("button");
      Writer.Key("value");
      Writer.String(Item.Value.c_str(), Item.Value.length());
      Writer.Key("replace_text");
      Writer.String(Item.ReplaceText.c_str(), Item.ReplaceText.length());
      constexpr size_t BufferSize = 8;
      constexpr size_t ColorHexLength = 6;
      thread_local char TextColorCStr[BufferSize];
      thread_local char BorderColorCStr[BufferSize];
      snprintf(TextColorCStr, sizeof(TextColorCStr), "%06X", Item.TextColor);
      snprintf(BorderColorCStr, sizeof(BorderColorCStr), "%06X", Item.BorderColor);
      Writer.Key("text_color");
      Writer.String(TextColorCStr, ColorHexLength);
      Writer.Key("border_color");
      Writer.String(BorderColorCStr, ColorHexLength);
      Writer.EndObject();
    }
    Writer.EndArray();
    Writer.EndObject();
    Writer.EndArray();
  }
  Writer.EndObject();
  // end
  Writer.EndObject();
  return std::string(SB.GetString(), SB.GetSize());
}

#define STR_APP(obj, sl) (obj.append(sl, sizeof(sl) - 1))

std::string MarkdownServerMessage::GetXml() const {
  std::string Xml;
  STR_APP(Xml, "<xml>");
  STR_APP(Xml, "<MsgType>text</MsgType>");
  // visible_to_user
  if (!VisibleToUser.empty()) {
    STR_APP(Xml, "<VisibleToUser>");
    thread_local std::string Buffer;
    Buffer.clear();
    for (const auto &Item : VisibleToUser) {
      Buffer.append(Item);
      Buffer.push_back('|');
    }
    Buffer.pop_back();  // omit last |
    Xml.append(Buffer);
    STR_APP(Xml, "</VisibleToUser>");
  }
  STR_APP(Xml, "<Markdown>");
  // content
  STR_APP(Xml, "<Content><![CDATA[");
  Xml.append(Content);
  STR_APP(Xml, "]]></Content>");
  // attachment
  if (!Actions.empty()) {
    STR_APP(Xml, "<Attachment>");
    STR_APP(Xml, "<CallbackId><![CDATA[");
    Xml.append(CallbackId);
    STR_APP(Xml, "]]></CallbackId>");
    for (const auto &Item : Actions) {
      STR_APP(Xml, "<Actions>");
      STR_APP(Xml, "<Name><![CDATA[");
      Xml.append(Item.Name);
      STR_APP(Xml, "]]></Name>");
      STR_APP(Xml, "<Value><![CDATA[");
      Xml.append(Item.Value);
      STR_APP(Xml, "]]></Value>");
      STR_APP(Xml, "<Text><![CDATA[");
      Xml.append(Item.Text);
      STR_APP(Xml, "]]></Text>");
      STR_APP(Xml, "<Type>button</Type>");
      constexpr size_t BufferSize = 8;
      constexpr size_t ColorHexLength = 6;
      thread_local char TextColorCStr[BufferSize];
      thread_local char BorderColorCStr[BufferSize];
      snprintf(TextColorCStr, sizeof(TextColorCStr), "%06X", Item.TextColor);
      snprintf(BorderColorCStr, sizeof(BorderColorCStr), "%06X", Item.BorderColor);
      STR_APP(Xml, "<BorderColor><![CDATA[");
      Xml.append(BorderColorCStr, ColorHexLength);
      STR_APP(Xml, "]]></BorderColor>");
      STR_APP(Xml, "<TextColor><![CDATA[");
      Xml.append(TextColorCStr, ColorHexLength);
      STR_APP(Xml, "]]></TextColor>");
      STR_APP(Xml, "<ReplaceText><![CDATA[");
      Xml.append(Item.ReplaceText);
      STR_APP(Xml, "]]></ReplaceText>");
      STR_APP(Xml, "</Actions>");
    }
    STR_APP(Xml, "</Attachment>");
  }
  STR_APP(Xml, "</Markdown>");
  STR_APP(Xml, "</xml>");
  return Xml;
}

}  // namespace wecom
}  // namespace wcbot
