#pragma once

#include "ServerMessage.h"

namespace wcbot {
namespace wecom {

class NewsServerMessage final : public ServerMessage {
 public:
  struct Article {
    std::string Title;        // max 128
    std::string Description;  // max 512
    std::string Url;
    std::string PicUrl;
  };
  std::vector<Article> Articles;  // max 8
  std::vector<std::string> VisibleToUser;
  std::string GetJson() const override;
  bool ValidateFields() const override;
};

}  // namespace wecom
}  // namespace wcbot
