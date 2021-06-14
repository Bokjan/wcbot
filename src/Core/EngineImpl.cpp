#include "EngineImpl.h"
#include "Utility/Common.h"
#include "UvBufferTcp.h"

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#define TRACE() fprintf(stderr, "F(%s) L(%d)\n", __FUNCTION__, __LINE__)

namespace wcbot {

namespace mlcb {
static void OnTcpRead(uv_stream_t *Handle, ssize_t NRead,
                      const uv_buf_t *Buffer);
static void OnTcpClose(uv_handle_t *Handle);
static void OnNewConnection(uv_stream_t *Handle, int Status);
static void AllocateBuffer(uv_handle_t *Handle, size_t SuggestedSize,
                           uv_buf_t *Buffer);
}  // namespace mlcb

#ifdef CHECK_INT_NOT_ZERO_RET
#undef CHECK_INT_NOT_ZERO_RET
#endif
#define CHECK_INT_NOT_ZERO_RET(x)          \
  do {                                     \
    int Check = x;                         \
    if (Check != 0) {                      \
      printf("L%d %d\n", __LINE__, Check); \
      return Check;                        \
    }                                      \
  } while (false);

EngineImpl::EngineImpl()
    : IsFork(true), StopSign(false), UvMainLoop(uv_default_loop()) {}

inline bool RapidJsonGetString(const rapidjson::Value &Value,
                               const char *Member, std::string &Destination) {
  if (!Value[Member].IsString()) {
    return false;
  }
  Destination =
      std::string(Value[Member].GetString(), Value[Member].GetStringLength());
  return true;
}

inline bool RapidJsonGetUInt64(const rapidjson::Value &Value,
                               const char *Member, uint64_t &Destination) {
  if (!Value[Member].IsUint64()) {
    return false;
  }
  Destination = Value[Member].GetUint64();
  return true;
}

#define BREAK_ON_FALSE(x) \
  if (!x) {               \
    break;                \
  }

static bool InternalParseConfig(BotConfig &Config,
                                const rapidjson::Document &Json) {
  Config.ParseOk = false;
  do {
    // Bot
    if (!Json["Bot"].IsObject()) {
      break;
    }
    const rapidjson::Value &Bot = Json["Bot"];
    BREAK_ON_FALSE(RapidJsonGetString(Bot, "WebHook", Config.Bot.WebHook));
    BREAK_ON_FALSE(RapidJsonGetString(Bot, "Token", Config.Bot.Token));
    BREAK_ON_FALSE(
        RapidJsonGetString(Bot, "EncodingAesKey", Config.Bot.EncodingAesKey));

    // Http
    if (!Json["Http"].IsObject()) {
      break;
    }
    const rapidjson::Value &Http = Json["Http"];
    BREAK_ON_FALSE(RapidJsonGetString(Http, "BindIpv4", Config.Http.BindIpv4));
    if (Http.HasMember("BindPort") && Http["BindPort"].IsInt()) {
      Config.Http.BindPort = Http["BindPort"].GetInt();
    } else {
      break;
    }

    // Network
    if (!Json["Network"].IsObject()) {
      break;
    }
    const rapidjson::Value &Network = Json["Network"];
    BREAK_ON_FALSE(RapidJsonGetUInt64(Network, "MaxRecvBuffLength",
                                      Config.Network.MaxRecvBuffLength));
    BREAK_ON_FALSE(RapidJsonGetUInt64(Network, "MaxSendBuffLength",
                                      Config.Network.MaxSendBuffLength));

    // CustomConfig
    if (Json.HasMember("CustomConfig")) {
      Config.CustomConfig = Json["CustomConfig"].GetString();
    }

    // Finish
    Config.ParseOk = true;
  } while (false);
  return Config.ParseOk;
}

bool EngineImpl::ParseConfig(const std::string &Path) {
  bool Ret = false;
  do {
    std::string ConfigContent;
    if (!utility::ReadFile(Path, ConfigContent)) {
      break;
    }
    rapidjson::Document Json;
    Json.Parse(ConfigContent.c_str());
    if (!Json.IsObject()) {
      break;
    }
    Ret = InternalParseConfig(this->Config, Json);
  } while (false);
  return Ret;
}

int EngineImpl::Run() {
  uv_tcp_t UvServerTcp;
  sockaddr_in SockAddr;
  CHECK_INT_NOT_ZERO_RET(uv_tcp_init(this->UvMainLoop, &UvServerTcp));
  CHECK_INT_NOT_ZERO_RET(uv_ip4_addr(Config.Http.BindIpv4.c_str(),
                                     Config.Http.BindPort, &SockAddr));
  CHECK_INT_NOT_ZERO_RET(
      uv_tcp_bind(&UvServerTcp, reinterpret_cast<sockaddr *>(&SockAddr), 0));
  constexpr int kDefaultBacklog = 128;
  CHECK_INT_NOT_ZERO_RET(
      uv_listen(reinterpret_cast<uv_stream_t *>(&UvServerTcp), kDefaultBacklog,
                mlcb::OnNewConnection));
  UvServerTcp.data = this;  // bind self
  int Ret = uv_run(this->UvMainLoop, UV_RUN_DEFAULT);
  printf("L%d %d\n", __LINE__, Ret);
  return Ret;
}

namespace mlcb {

static void OnNewConnection(uv_stream_t *Tcp, int Status) {
  TRACE();
  if (Status < 0) {
    // todo: error log
    return;
  }
  EngineImpl *Impl = reinterpret_cast<EngineImpl *>(Tcp->data);
  UvBufferTcpPtr Buffer = new UvBufferTcp();
  Buffer->ServerTcp = reinterpret_cast<uv_tcp_t *>(Tcp);
  uv_tcp_init(Impl->UvMainLoop, &Buffer->ClientTcp);
  Buffer->ClientTcp.data = Buffer;
  if (uv_accept(Tcp, reinterpret_cast<uv_stream_t *>(&Buffer->ClientTcp)) ==
      0) {
    uv_read_start(reinterpret_cast<uv_stream_t *>(&Buffer->ClientTcp),
                  mlcb::AllocateBuffer, mlcb::OnTcpRead);
  } else {
    uv_close(reinterpret_cast<uv_handle_t *>(&Buffer->ClientTcp),
             mlcb::OnTcpClose);
  }
}

static void OnTcpClose(uv_handle_t *Handle) {
  TRACE();
  void *UserData = Handle->data;
  delete reinterpret_cast<UvBufferTcpPtr>(UserData);
}

static void AllocateBuffer(uv_handle_t *Handle, size_t SuggestedSize,
                           uv_buf_t *Buffer) {
  TRACE();
  UvBufferTcpPtr BufferObj = reinterpret_cast<UvBufferTcpPtr>(Handle->data);
  BufferObj->Allocate(SuggestedSize);
  Buffer->base = BufferObj->GetCurrent();
  Buffer->len = SuggestedSize;
  fprintf(stderr, "%p, %lu\n", Buffer->base, Buffer->len);
}

static void OnTcpRead(uv_stream_t *Handle, ssize_t NRead,
                      const uv_buf_t *Buffer) {
  TRACE();
  UvBufferTcpPtr BufferObj = reinterpret_cast<UvBufferTcpPtr>(Handle->data);
  EngineImpl *PImpl =
      reinterpret_cast<EngineImpl *>(BufferObj->ServerTcp->data);
  // currently used buffer size is larger than configured value
  if (BufferObj->GetCapacity() > PImpl->Config.Network.MaxRecvBuffLength) {
    TRACE();
    uv_close(reinterpret_cast<uv_handle_t *>(Handle), mlcb::OnTcpClose);
    return;
  }
  // check size is ok
  if (NRead < 0) {
    if (NRead != UV_EOF) {
      // todo: log
    }
    TRACE();
    uv_close(reinterpret_cast<uv_handle_t *>(Handle), mlcb::OnTcpClose);
    return;
  }
  // just increase the buffer length value
  BufferObj->IncreaseLength(NRead);
  // run codec
  for (auto *CodecPtr : PImpl->ServerCodecs) {
    size_t ValidLength = CodecPtr->Check(*BufferObj);
    if (ValidLength > 0) {
      // todo: dispatch
      break;
    }
  }
}

}  // namespace mlcb

}  // namespace wcbot
