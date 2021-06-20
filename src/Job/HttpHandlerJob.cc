#include "HttpHandlerJob.h"

#include <cinttypes>

#include <map>
#include <sstream>
#include <vector>

#include "../Core/Engine.h"
#include "../Core/EngineImpl.h"
#include "../ThirdParty/WXBizMsgCrypt/WXBizMsgCrypt.h"
#include "../Utility/Common.h"
#include "SilentPushJob.h"

namespace wcbot {

HttpHandlerJob::HttpHandlerJob(TcpMemoryBuffer* RB)
    : TcpHandlerJob(RB), State(StateEnum::kStart) {}

void HttpHandlerJob::OnTimeout(Job* Trigger) {
  this->Response504GatewayTimeout();
  DeleteThis();
}

void HttpHandlerJob::Do(Job* Trigger) {
  switch (State) {
    case StateEnum::kStart:
      this->DoStart();
      break;
    case StateEnum::kParseTcpPackage:
      this->DoParseTcpPackage();
      break;
    case StateEnum::kDispatchRequest:
      this->DoDispatchRequest();
      break;
    case StateEnum::kVerifyCallbackSetting:
      this->DoVerifyCallbackSetting();
      break;
    case StateEnum::kFinish:
      this->DoFinish();
      break;
    default:
      break;
  }
}

void HttpHandlerJob::DoStart() {
  State = StateEnum::kParseTcpPackage;
  this->Do();
}

void HttpHandlerJob::DoFinish() { DeleteThis(); }

void HttpHandlerJob::DoParseTcpPackage() {
  bool Success = Request.Parse(ReceiveBuffer->GetBase(), ReceiveBuffer->GetLength());
  if (!Success) {
    this->Response400BadRequest();
    State = StateEnum::kFinish;
  } else {
    State = StateEnum::kDispatchRequest;
  }
  this->Do();
}

void HttpHandlerJob::DoDispatchRequest() {
  // verify callback?
  if (Request.Method == HttpRequest::MethodEnum::kGet &&
      Request.Path == Engine::Get().GetImpl().Config.Bot.CallbackVerifyPath) {
    State = StateEnum::kVerifyCallbackSetting;
    this->Do();
    return;
  }
  // todo
  // wecom::TextServerMessage TSM;
  // TSM.Content = "hello world from C++";
  // auto J = new SilentPushJob(this->Worker, TSM);
  // J->Do();
  this->Response400BadRequest();
  State = StateEnum::kFinish;
  this->Do();
}

static void SplitString(const std::string& Input, std::vector<std::string>& Output,
                        char Delimeter) {
  Output.clear();
  std::string Buffer;
  std::stringstream SS(Input);
  while (std::getline(SS, Buffer, Delimeter)) {
    Output.emplace_back(Buffer);
  }
}

static void GetQueryStringKV(const std::string& Input, std::map<std::string, std::string>& Output) {
  thread_local std::vector<std::string> KVStrings;
  KVStrings.clear();
  Output.clear();
  SplitString(Input, KVStrings, '&');
  for (const auto& KV : KVStrings) {
    auto Position = KV.find_first_of('=');
    if (Position == std::string::npos) {
      continue;
    }
    Output.insert(
        std::make_pair(std::string(KV.c_str(), Position),
                       std::string(KV.c_str() + Position + 1, KV.length() - Position - 1)));
  }
}

void HttpHandlerJob::DoVerifyCallbackSetting() {
  thread_local std::map<std::string, std::string> KVPairs;
  KVPairs.clear();
  std::string UrlDecoded = utility::UrlDecode(Request.QueryString);
  GetQueryStringKV(UrlDecoded, KVPairs);
  std::string Decrypted;
  Decrypted.clear();
  int Ret =
      Engine::Get().GetImpl().Cryptor->VerifyURL(KVPairs["msg_signature"], KVPairs["timestamp"],
                                                 KVPairs["nonce"], KVPairs["echostr"], Decrypted);
  if (Ret != 0) {
    LOG_WARN("WXBizMsgCrypt::VerifyURL ret=%d, qs(decoded)=%s", Ret, UrlDecoded.c_str());
    this->Response400BadRequest();
  } else {
    LOG_DEBUG("decrypted=%s", Decrypted.c_str());
    this->ResponseVerifyEchoString(Decrypted);
  }
  State = StateEnum::kFinish;
  this->Do();
}

void HttpHandlerJob::Response400BadRequest() {
  MemoryBuffer* MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
  this->SendData(MB, kDisconnect);
}

void HttpHandlerJob::Response504GatewayTimeout() {
  MemoryBuffer* MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 504 Gateway Timeout\r\nContent-Length: 0\r\n\r\n");
  this->SendData(MB, kDisconnect);
}

void HttpHandlerJob::ResponseVerifyEchoString(const std::string& Body) {
  MemoryBuffer* MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 200 OK\r\nContent-Length: ");
  char PrintBuffer[32];
  int PrintLength = snprintf(PrintBuffer, sizeof(PrintBuffer), "%" PRIu64, Body.length());
  MB->Append(PrintBuffer, PrintLength);
  MEMBUF_APP(MB, "\r\n\r\n");
  MB->Append(Body);
  this->SendData(MB, kDisconnect);
}

}  // namespace wcbot
