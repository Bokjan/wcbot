#include "NewsServerMessage.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../Utility/Logger.h"

namespace wcbot {
namespace wecom {

bool NewsServerMessage::ValidateFields() const {
  constexpr decltype(Articles.size()) kMaxArticleCount = 8;
  bool Return = false;
  do {
    if (Articles.empty()) {
      LOG_WARN("%s", "NewsServerMessage doesn't have any article");
      break;
    }
    if (Articles.size() > kMaxArticleCount) {
      LOG_WARN("NewsServerMessage Articles.size=%" PRIu64 ", larger than %" PRIu64, Articles.size(),
               kMaxArticleCount);
      break;
    }
    if (!VisibleToUser.empty()) {
      if (ChatId.empty()) {
        LOG_WARN("%s", "VisibleToUser set but ChatId is empty");
        // don't break
      }
    }
    Return = true;
  } while (false);
  return Return;
}

std::string NewsServerMessage::GetJson() const {
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
  // visible to user
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
  Writer.String("news");
  Writer.Key("news");
  Writer.StartObject();
  Writer.Key("articles");
  Writer.StartArray();
  for (const auto &Item : Articles) {
    Writer.StartObject();
    Writer.Key("title");
    Writer.String(Item.Title.c_str(), Item.Title.length());
    Writer.Key("description");
    Writer.String(Item.Description.c_str(), Item.Description.length());
    Writer.Key("url");
    Writer.String(Item.Url.c_str(), Item.Url.length());
    Writer.Key("picurl");
    Writer.String(Item.PicUrl.c_str(), Item.PicUrl.length());
    Writer.EndObject();
  }
  Writer.EndArray();
  Writer.EndObject();
  // end
  Writer.EndObject();
  return std::string(SB.GetString(), SB.GetSize());
}

}  // namespace wecom
}  // namespace wcbot
