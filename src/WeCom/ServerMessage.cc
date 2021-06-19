#include "ServerMessage.h"

#include <string>

namespace wcbot {

class MemoryBuffer;

namespace wecom {

ServerMessage::~ServerMessage() {}

MemoryBuffer* ServerMessage::GetXml() const { return nullptr; }

}

}
