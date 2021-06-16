#pragma once

#include "UvBuffer.h"

#include <uv.h>

namespace wcbot {

class TcpUvBuffer : public UvBuffer {
 public:
  uv_tcp_t ClientTcp;
  uv_tcp_t *ServerTcp;
};
using TcpUvBufferPtr = TcpUvBuffer *;

}  // namespace wcbot
