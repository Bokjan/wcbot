#include "HttpHandlerJob.h"

#include <cinttypes>
#include <cstdio>
#include <ctime>

#include <map>
#include <sstream>
#include <vector>

#include "../Core/Engine.h"
#include "../Core/EngineImpl.h"
#include "../Job/MessageCallbackJob.h"
#include "../ThirdParty/WXBizMsgCrypt/WXBizMsgCrypt.h"
#include "../Utility/Common.h"
#include "../WeCom/ClientMessageImpl.h"

namespace wcbot {

HttpHandlerJob::HttpHandlerJob(TcpMemoryBuffer* RB) : TcpHandlerJob(RB), State(StateEnum::kStart) {}

Job::Step HttpHandlerJob::OnStep(Job* Trigger) {
  switch (State) {
    case StateEnum::kStart:
      State = StateEnum::kParseTcpPackage;
      return Step::kContinue;

    case StateEnum::kParseTcpPackage:
      return DoParseTcpPackage();

    case StateEnum::kDispatchRequest:
      return DoDispatchRequest();

    case StateEnum::kVerifyCallbackSetting:
      return DoVerifyCallbackSetting();

    case StateEnum::kInvokeCallbackJobStart:
      return DoInvokeCallbackJobStart();

    case StateEnum::kInvokeCallbackJobFinish:
      return DoInvokeCallbackJobFinish(Trigger);

    case StateEnum::kFinish:
      return Step::kDone;
  }
  return Step::kDone;
}

Job::Step HttpHandlerJob::DoParseTcpPackage() {
  bool Success = Request.Parse(ReceiveBuffer->GetBase(), ReceiveBuffer->GetLength());
  if (!Success) {
    Response400BadRequest();
    State = StateEnum::kFinish;
    return Step::kContinue;
  }
  State = StateEnum::kDispatchRequest;
  return Step::kContinue;
}

Job::Step HttpHandlerJob::DoDispatchRequest() {
  // callback message?
  if (Request.Method == HttpRequest::MethodEnum::kPost &&
      Request.Path == Engine::Get().GetImpl().Config.Bot.CallbackPath) {
    State = StateEnum::kInvokeCallbackJobStart;
    return Step::kContinue;
  }
  // verify callback?
  if (Request.Method == HttpRequest::MethodEnum::kGet &&
      Request.Path == Engine::Get().GetImpl().Config.Bot.CallbackPath) {
    State = StateEnum::kVerifyCallbackSetting;
    return Step::kContinue;
  }
  Response400BadRequest();
  State = StateEnum::kFinish;
  return Step::kContinue;
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

Job::Step HttpHandlerJob::DoVerifyCallbackSetting() {
  thread_local std::map<std::string, std::string> KVPairs;
  KVPairs.clear();
  std::string UrlDecoded = utility::UrlDecode(Request.QueryString);
  GetQueryStringKV(UrlDecoded, KVPairs);
  thread_local std::string Decrypted;
  Decrypted.clear();
  int Ret =
      Engine::Get().GetImpl().Cryptor->VerifyURL(KVPairs["msg_signature"], KVPairs["timestamp"],
                                                 KVPairs["nonce"], KVPairs["echostr"], Decrypted);
  if (Ret != 0) {
    LOG_WARN("WXBizMsgCrypt::VerifyURL ret=%d, qs(decoded)=%s", Ret, UrlDecoded.c_str());
    Response400BadRequest();
  } else {
    Response200OK(Decrypted);
  }
  State = StateEnum::kFinish;
  return Step::kContinue;
}

Job::Step HttpHandlerJob::DoInvokeCallbackJobStart() {
  if (!Engine::Get().GetImpl().CbHandlerCreator) {
    Response501NotImplemented();
    State = StateEnum::kFinish;
    return Step::kContinue;
  }
  thread_local std::map<std::string, std::string> KVPairs;
  KVPairs.clear();
  std::string UrlDecoded = utility::UrlDecode(Request.QueryString);
  GetQueryStringKV(UrlDecoded, KVPairs);
  thread_local std::string Decrypted;
  Decrypted.clear();
  int Ret = Engine::Get().GetImpl().Cryptor->DecryptMsg(
      KVPairs["msg_signature"], KVPairs["timestamp"], KVPairs["nonce"], Request.Body, Decrypted);
  if (Ret != 0) {
    LOG_WARN("WXBizMsgCrypt::DecryptMsg ret=%d", Ret);
    Response400BadRequest();
    State = StateEnum::kFinish;
    return Step::kContinue;
  }
  auto ClientMsg = wecom::client_message_impl::GenerateClientMessageByXml(Decrypted);
  if (ClientMsg == nullptr) {
    LOG_WARN("%s", "wecom::client_message_impl::GenerateClientMessageByXml failed");
    Response400BadRequest();
    State = StateEnum::kFinish;
    return Step::kContinue;
  }
  auto* Child = Engine::Get().GetImpl().CbHandlerCreator();
  Child->SetRequest(std::move(ClientMsg));
  State = StateEnum::kInvokeCallbackJobFinish;
  InvokeChild(Child);
  return Step::kWaiting;
}

static bool GetResponseBodyByCallbackMessage(MessageCallbackJob* J, std::string& Encrypted) {
  auto Xml = J->GetResponse()->GetXml();
  char Nonce[32];
  char Timestamp[32];
  snprintf(Nonce, sizeof(Nonce), "%u", utility::ThreadLocalRand());
  snprintf(Timestamp, sizeof(Timestamp), "%ld", time(nullptr));
  int Ret = Engine::Get().GetImpl().Cryptor->EncryptMsg(Xml, Timestamp, Nonce, Encrypted);
  if (Ret != 0) {
    LOG_ERROR("WXBizMsgCrypt::EncryptMsg failed, ret=%d, nonce=%s, timestamp=%s", Ret, Nonce,
              Timestamp);
    return false;
  }
  return true;
}

Job::Step HttpHandlerJob::DoInvokeCallbackJobFinish(Job* ChildBase) {
  auto* Child = AsJob<MessageCallbackJob>(ChildBase);
  do {
    if (Child == nullptr) {
      LOG_ERROR("%s", "HttpHandlerJob: trigger is not a MessageCallbackJob");
      Response500InternalServerError();
      break;
    }
    if (Child->GetResponse() == nullptr) {
      // WeCom allows a temporary 200 OK with empty body; the actual reply
      // can be pushed back via the webhook URL later.
      Response200OK("");
      break;
    }
    thread_local std::string Encrypted;
    Encrypted.clear();
    bool Check = GetResponseBodyByCallbackMessage(Child, Encrypted);
    if (Check) {
      Response200OK(Encrypted);
    } else {
      Response500InternalServerError();
    }
  } while (false);
  State = StateEnum::kFinish;
  return Step::kContinue;
}

void HttpHandlerJob::Response200OK(const std::string& Body) {
  MemoryBuffer* MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 200 OK\r\nContent-Length: ");
  char PrintBuffer[32];
  int PrintLength =
      snprintf(PrintBuffer, sizeof(PrintBuffer), "%" PRIu64, static_cast<uint64_t>(Body.length()));
  MB->Append(PrintBuffer, PrintLength);
  MEMBUF_APP(MB, "\r\n\r\n");
  MB->Append(Body);
  this->SendData(MB, kDisconnect);
}

void HttpHandlerJob::Response400BadRequest() {
  MemoryBuffer* MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
  this->SendData(MB, kDisconnect);
}

void HttpHandlerJob::Response500InternalServerError() {
  MemoryBuffer* MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n");
  this->SendData(MB, kDisconnect);
}

void HttpHandlerJob::Response501NotImplemented() {
  MemoryBuffer* MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 501 Not Implemented\r\nContent-Length: 0\r\n\r\n");
  this->SendData(MB, kDisconnect);
}

void HttpHandlerJob::Response504GatewayTimeout() {
  MemoryBuffer* MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 504 Gateway Timeout\r\nContent-Length: 0\r\n\r\n");
  this->SendData(MB, kDisconnect);
}

}  // namespace wcbot