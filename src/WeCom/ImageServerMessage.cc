#include "ImageServerMessage.h"

#include <cinttypes>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../Utility/Common.h"
#include "../Utility/Logger.h"

namespace wcbot {
namespace wecom {

bool ImageServerMessage::ValidateFields() const {
  constexpr decltype(ImageData.length()) kImageMaxSize = 2 * 1024 * 1024;
  if (ImageData.empty()) {
    LOG_WARN("%s", "ImageServerMessage ImageData is empty!");
    return false;
  }
  if (ImageData.length() > kImageMaxSize) {
    LOG_WARN("ImageServerMessage ImageData.length=%" PRIu64 ", larger than %" PRIu64,
             ImageData.length(), kImageMaxSize);
    return false;
  }
  // only JPG and PNG are OK, but not checked
  return true;
}

std::string ImageServerMessage::GetJson() const {
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
  Writer.String("image");
  Writer.Key("image");
  Writer.StartObject();
  thread_local std::string MD5;
  thread_local std::string Base64;
  MD5 = utility::Md5String(ImageData.data(), ImageData.size());
  Base64 = utility::Base64Encode(ImageData.data(), ImageData.size());
  Writer.Key("md5");
  Writer.String(MD5.c_str(), MD5.length());
  Writer.Key("base64");
  Writer.String(Base64.c_str(), Base64.length());
  Writer.EndObject();
  // end
  Writer.EndObject();
  return std::string(SB.GetString(), SB.GetSize());
}

}  // namespace wecom
}  // namespace wcbot
