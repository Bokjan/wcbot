#pragma once

#include <uv.h>

#include "MemoryBuffer.h"

namespace wcbot {

class TcpMemoryBuffer : public MemoryBuffer {
 public:
  uint64_t ClientTcpId;
  uv_tcp_t *ServerTcp;
  TcpMemoryBuffer(uint64_t C, uv_tcp_t *S) : ClientTcpId(C), ServerTcp(S) {}
  TcpMemoryBuffer(const TcpMemoryBuffer &) = delete;
  TcpMemoryBuffer(const TcpMemoryBuffer &&) = delete;
};

}  // namespace wcbot
