#include "FileServerMessage.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace wcbot {
namespace wecom {

bool FileServerMessage::ValidateFields() const { return true; }

std::string FileServerMessage::GetJson() const {
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
  Writer.String("file");
  Writer.Key("file");
  Writer.StartObject();
  Writer.Key("media_id");
  Writer.String(MediaId.c_str(), MediaId.length());
  Writer.EndObject();
  // end
  Writer.EndObject();
  return std::string(SB.GetString(), SB.GetSize());
}

}  // namespace wecom
}  // namespace wcbot
