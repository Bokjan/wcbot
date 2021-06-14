#pragma once

#include <string>

namespace wcbot {
namespace utility {

bool ReadFile(const std::string& Path, std::string& ReceiveBuffer);

}
}  // namespace wcbot
