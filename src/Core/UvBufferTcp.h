#pragma once

#include "UvBuffer.h"

#include <uv.h>

namespace wcbot {

class UvBufferTcp : public UvBuffer {
 public:
  uv_tcp_t ClientTcp;
  uv_tcp_t *ServerTcp;
};
using UvBufferTcpPtr = UvBufferTcp *;

}  // namespace wcbot
