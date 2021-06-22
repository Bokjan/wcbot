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
#include "../Job/SilentPushJob.h"
#include "../ThirdParty/WXBizMsgCrypt/WXBizMsgCrypt.h"
#include "../Utility/Common.h"
#include "../WeCom/ClientMessageImpl.h"

namespace wcbot {

HttpHandlerJob::HttpHandlerJob(TcpMemoryBuffer* RB) : TcpHandlerJob(RB), State(StateEnum::kStart) {}

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
    case StateEnum::kInvokeCallbackJobStart:
      this->DoInvokeCallbackJobStart();
      break;
    case StateEnum::kInvokeCallbackJobFinish:
      this->DoInvokeCallbackJobFinish(Trigger);
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
  do {
    // callback message?
    if (Request.Method == HttpRequest::MethodEnum::kPost &&
        Request.Path == Engine::Get().GetImpl().Config.Bot.CallbackPath) {
      State = StateEnum::kInvokeCallbackJobStart;
      this->Do();
      break;
    }
    // verify callback?
    if (Request.Method == HttpRequest::MethodEnum::kGet &&
        Request.Path == Engine::Get().GetImpl().Config.Bot.CallbackPath) {
      State = StateEnum::kVerifyCallbackSetting;
      this->Do();
      break;
    }
    // else
    this->Response400BadRequest();
    State = StateEnum::kFinish;
    this->Do();
  } while (false);
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
  thread_local std::string Decrypted;
  Decrypted.clear();
  int Ret =
      Engine::Get().GetImpl().Cryptor->VerifyURL(KVPairs["msg_signature"], KVPairs["timestamp"],
                                                 KVPairs["nonce"], KVPairs["echostr"], Decrypted);
  if (Ret != 0) {
    LOG_WARN("WXBizMsgCrypt::VerifyURL ret=%d, qs(decoded)=%s", Ret, UrlDecoded.c_str());
    this->Response400BadRequest();
  } else {
    this->Response200OK(Decrypted);
  }
  State = StateEnum::kFinish;
  this->Do();
}

void HttpHandlerJob::DoInvokeCallbackJobStart() {
  // handler registered?
  if (Engine::Get().GetImpl().CbHandlerCreator == nullptr) {
    this->Response501NotImplemented();
    State = StateEnum::kFinish;
    this->Do();
    return;
  }
  // decrypt request
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
    this->Response400BadRequest();
    State = StateEnum::kFinish;
    this->Do();
    return;
  }
  // LOG_DEBUG("%s", Decrypted.c_str());
  // parse xml
  auto* ClientMsg = wecom::client_message_impl::GenerateClientMessageByXml(Decrypted);
  if (ClientMsg == nullptr) {
    LOG_WARN("%s", "wecom::client_message_impl::GenerateClientMessageByXml failed");
    this->Response400BadRequest();
    State = StateEnum::kFinish;
    this->Do();
    return;
  }
  // create a handler and invoke
  auto* Child = Engine::Get().GetImpl().CbHandlerCreator();
  Child->SetRequest(ClientMsg);
  State = StateEnum::kInvokeCallbackJobFinish;
  InvokeChild(Child);
}

static bool GetResponseBodyByCallbackMessage(MessageCallbackJob* J, std::string& Encrypted) {
  auto Xml = J->GetResponse()->GetXml();
  char Nonce[32];
  char Timestamp[32];
  snprintf(Nonce, sizeof(Nonce), "%d", rand());
  snprintf(Timestamp, sizeof(Timestamp), "%ld", time(nullptr));
  int Ret = Engine::Get().GetImpl().Cryptor->EncryptMsg(Xml, Timestamp, Nonce, Encrypted);
  if (Ret != 0) {
    LOG_ERROR("WXBizMsgCrypt::EncryptMsg failed, ret=%d, nonce=%s, timestamp=%s", Ret, Nonce,
              Timestamp);
    return false;
  }
  return true;
}

void HttpHandlerJob::DoInvokeCallbackJobFinish(Job* ChildBase) {
  auto* Child = dynamic_cast<MessageCallbackJob*>(ChildBase);
  do {
    // class type doesn't match?
    if (Child == nullptr) {
      LOG_ERROR("%s", "HttpHandlerJob dynamic_cast<MessageCallbackJob*>(ChildBase) failed");
      this->Response500InternalServerError();
      break;
    }
    // response ptr is null?
    if (Child->GetResponse() == nullptr) {
      // WeCom allows user to temporarily reply a empty 200 OK
      // then send the actual reply by `WebhookUrl`
      LOG_TRACE("%s", "response 200 OK with empty body");
      this->Response200OK("");
      break;
    }
    // get the XML, encrypt and reply
    thread_local std::string Encrypted;
    Encrypted.clear();
    bool Check = GetResponseBodyByCallbackMessage(Child, Encrypted);
    if (Check) {
      this->Response200OK(Encrypted);
    } else {
      this->Response500InternalServerError();
    }
  } while (false);
  State = StateEnum::kFinish;
  this->Do();
}

void HttpHandlerJob::Response200OK(const std::string& Body) {
  LOG_TRACE("%s", Body.c_str());
  MemoryBuffer* MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 200 OK\r\nContent-Length: ");
  char PrintBuffer[32];
  int PrintLength = snprintf(PrintBuffer, sizeof(PrintBuffer), "%" PRIu64, Body.length());
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
