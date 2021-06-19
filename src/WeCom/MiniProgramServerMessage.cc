#include "MiniProgramServerMessage.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace wcbot {
namespace wecom {

std::string MiniProgramServerMessage::GetJson() const {
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
  // msgtype
  Writer.Key("msgtype");
  Writer.String("miniprogram");
  Writer.Key("miniprogram");
  Writer.StartObject();
  Writer.Key("title");
  Writer.String(Title.c_str(), Title.length());
  Writer.Key("pic_media_id");
  Writer.String(MediaId.c_str(), MediaId.length());
  Writer.Key("appid");
  Writer.String(AppId.c_str(), AppId.length());
  Writer.Key("page");
  Writer.String(Page.c_str(), Page.length());
  Writer.EndObject();
  // end
  Writer.EndObject();
  return std::string(SB.GetString(), SB.GetSize());
}

bool MiniProgramServerMessage::ValidateFields() const { return true; }

}  // namespace wecom
}  // namespace wcbot
