#include "Common.h"

#include <fstream>
#include <sstream>
// #include <streambuf>

namespace wcbot {
namespace utility {

bool ReadFile(const std::string& Path, std::string& ReceiveBuffer) {
  std::ifstream Stream;
  Stream.open(Path);
  if (!Stream.is_open()) {
    return false;
  }
  std::stringstream SSBuffer;
  SSBuffer << Stream.rdbuf();
  ReceiveBuffer = SSBuffer.str();
  return true;
}

}  // namespace utility
}  // namespace wcbot
