#include "DemoCallbackJob.h"

#include <cctype>
#include <cstdlib>

#include <memory>

#include "wcbot/Job/HttpClientJob.h"
#include "wcbot/Utility/Logger.h"
#include "wcbot/WeCom/TextClientMessage.h"
#include "wcbot/WeCom/TextServerMessage.h"

namespace {

constexpr int kHttpTimeoutMs = 4500;     // stay under WeCom's 5s callback budget
constexpr uint32_t kMaxSleepMs = 4000;   // ditto
constexpr const char* kHelpText =
    "wcbot demo commands:\n"
    "  /help          show this help\n"
    "  /ping          reply \"pong\"\n"
    "  /uuid          fetch a UUID from httpbin.org (demo: child IO job)\n"
    "  /sleep <ms>    delay reply by <ms> (max 4000)\n"
    "  (anything else) echo verbatim";

// Trim ASCII whitespace from both ends, in place.
void TrimAscii(std::string& S) {
  size_t Begin = 0;
  while (Begin < S.size() && std::isspace(static_cast<unsigned char>(S[Begin]))) {
    ++Begin;
  }
  size_t End = S.size();
  while (End > Begin && std::isspace(static_cast<unsigned char>(S[End - 1]))) {
    --End;
  }
  if (Begin > 0 || End < S.size()) {
    S = S.substr(Begin, End - Begin);
  }
}

// Build the synchronous text reply body.
std::unique_ptr<wcbot::wecom::TextServerMessage> MakeTextReply(const std::string& Content) {
  auto TSM = std::unique_ptr<wcbot::wecom::TextServerMessage>(
      new wcbot::wecom::TextServerMessage());
  TSM->Content = Content;
  return TSM;
}

}  // namespace

DemoCallbackJob::DemoCallbackJob()
    : State(StateEnum::kStart),
      Command(CommandEnum::kIgnore),
      SleepMs(0),
      HttpChild(nullptr) {}

DemoCallbackJob::~DemoCallbackJob() = default;

DemoCallbackJob::Step DemoCallbackJob::OnStep(wcbot::Job* Trigger) {
  switch (State) {
    case StateEnum::kStart:
      return DoStart(Trigger);
    case StateEnum::kIssueHttp:
      return DoIssueHttp(Trigger);
    case StateEnum::kAwaitSleep:
      return DoAwaitSleep(Trigger);
    case StateEnum::kFinish:
      return DoFinish();
  }
  return Step::kDone;
}

void DemoCallbackJob::OnCancel() {
  // Parent HttpHandlerJob got cancelled (peer disconnect, server shutdown,
  // or business decision). The framework will recursively cancel any
  // children we still hold (e.g. the HttpClientJob spawned by /uuid) and
  // then delete this object. We don't touch HttpChild here — it's owned
  // by the framework now and would be a use-after-free hazard.
  LOG_INFO("DemoCallbackJob::OnCancel cmd=%d state=%d",
           static_cast<int>(Command), static_cast<int>(State));
}

DemoCallbackJob::Step DemoCallbackJob::DoStart(wcbot::Job* /*Trigger*/) {
  auto* TCM = dynamic_cast<wcbot::wecom::TextClientMessage*>(Request.get());
  if (TCM == nullptr) {
    // Non-text inbound — leave Response unset; framework replies 200 OK
    // with empty body, matching WeCom's "I'll push later" convention.
    return Step::kDone;
  }

  // Parse the leading slash-command, if any.
  const std::string& Raw = TCM->Content;
  std::string Trimmed = Raw;
  TrimAscii(Trimmed);

  auto StartsWith = [&Trimmed](const char* Prefix) {
    size_t L = 0;
    while (Prefix[L] != '\0') {
      ++L;
    }
    if (Trimmed.size() < L) {
      return false;
    }
    return Trimmed.compare(0, L, Prefix) == 0 &&
           (Trimmed.size() == L || std::isspace(static_cast<unsigned char>(Trimmed[L])));
  };

  if (StartsWith("/help")) {
    Command = CommandEnum::kHelp;
    ReplyContent = kHelpText;
    State = StateEnum::kFinish;
    return Step::kContinue;
  }
  if (StartsWith("/ping")) {
    Command = CommandEnum::kPing;
    ReplyContent = "pong";
    State = StateEnum::kFinish;
    return Step::kContinue;
  }
  if (StartsWith("/uuid")) {
    Command = CommandEnum::kUuid;
    HttpChild = new wcbot::HttpClientJob();  // framework will own this
    HttpChild->Request.SetUrl("https://httpbin.org/uuid");
    HttpChild->Request.Method = wcbot::HttpRequest::MethodEnum::kGet;
    HttpChild->TimeoutMS = kHttpTimeoutMs;
    State = StateEnum::kIssueHttp;
    InvokeChild(HttpChild);
    return Step::kWaiting;
  }
  if (StartsWith("/sleep")) {
    Command = CommandEnum::kSleep;
    // /sleep<spaces><digits>
    Argument = Trimmed.substr(6);
    TrimAscii(Argument);
    long Ms = std::strtol(Argument.c_str(), nullptr, 10);
    if (Ms < 0) {
      Ms = 0;
    }
    if (Ms > static_cast<long>(kMaxSleepMs)) {
      Ms = kMaxSleepMs;
    }
    SleepMs = static_cast<uint32_t>(Ms);
    State = StateEnum::kAwaitSleep;
    Sleep(static_cast<int>(SleepMs));
    return Step::kWaiting;
  }

  // No recognised command — echo verbatim.
  Command = CommandEnum::kEcho;
  ReplyContent = Raw;
  State = StateEnum::kFinish;
  return Step::kContinue;
}

DemoCallbackJob::Step DemoCallbackJob::DoIssueHttp(wcbot::Job* Trigger) {
  auto* Child = AsJob<wcbot::HttpClientJob>(Trigger);
  if (Child == nullptr) {
    // Should never happen — Trigger must be the child we spawned.
    ReplyContent = "internal error: trigger is not an HttpClientJob";
    State = StateEnum::kFinish;
    return Step::kContinue;
  }

  if (Child->ErrCode == kErrTimeout) {
    LOG_WARN("DemoCallbackJob: /uuid timed out after %d ms", kHttpTimeoutMs);
    ReplyContent = "uuid lookup timed out";
  } else if (Child->Response.StatusCode != 200) {
    LOG_WARN("DemoCallbackJob: /uuid http status=%d errcode=%d",
             Child->Response.StatusCode, Child->ErrCode);
    ReplyContent = "uuid lookup failed (http " +
                   std::to_string(Child->Response.StatusCode) + ")";
  } else {
    std::string Uuid;
    if (ExtractJsonString(Child->Response.Body, "uuid", Uuid)) {
      ReplyContent = "uuid: " + Uuid;
    } else {
      ReplyContent = "uuid lookup succeeded but response was unparseable";
    }
  }

  // Child will be deleted by the framework right after this step returns;
  // drop our cached pointer to make a dangling read impossible.
  HttpChild = nullptr;
  State = StateEnum::kFinish;
  return Step::kContinue;
}

DemoCallbackJob::Step DemoCallbackJob::DoAwaitSleep(wcbot::Job* /*Trigger*/) {
  ReplyContent = "slept " + std::to_string(SleepMs) + "ms";
  State = StateEnum::kFinish;
  return Step::kContinue;
}

DemoCallbackJob::Step DemoCallbackJob::DoFinish() {
  if (!ReplyContent.empty()) {
    SetResponse(MakeTextReply(ReplyContent));
  }
  return Step::kDone;
}

// -----------------------------------------------------------------------------
// Tiny zero-dependency JSON string-field extractor.
//
// Looks for `"<Key>"` (top-level, naive — does not understand nested braces)
// followed by `:` and a JSON string literal. Handles \" \\ \/ \n \r \t \b \f
// and leaves \uXXXX as-is (not needed for httpbin.org's UUID endpoint).
// Returns false on any structural anomaly.
// -----------------------------------------------------------------------------
bool DemoCallbackJob::ExtractJsonString(const std::string& Body, const std::string& Key,
                                       std::string& Out) {
  Out.clear();
  // Locate `"<Key>"` — naive but sufficient for the demo.
  std::string Needle;
  Needle.reserve(Key.size() + 2);
  Needle.push_back('"');
  Needle.append(Key);
  Needle.push_back('"');
  size_t KeyPos = Body.find(Needle);
  if (KeyPos == std::string::npos) {
    return false;
  }
  size_t Cursor = KeyPos + Needle.size();
  // Skip whitespace + colon + whitespace.
  while (Cursor < Body.size() && std::isspace(static_cast<unsigned char>(Body[Cursor]))) {
    ++Cursor;
  }
  if (Cursor >= Body.size() || Body[Cursor] != ':') {
    return false;
  }
  ++Cursor;
  while (Cursor < Body.size() && std::isspace(static_cast<unsigned char>(Body[Cursor]))) {
    ++Cursor;
  }
  if (Cursor >= Body.size() || Body[Cursor] != '"') {
    return false;
  }
  ++Cursor;
  // Read until the closing un-escaped quote.
  while (Cursor < Body.size()) {
    char C = Body[Cursor];
    if (C == '"') {
      return true;
    }
    if (C == '\\') {
      if (Cursor + 1 >= Body.size()) {
        return false;
      }
      char Esc = Body[Cursor + 1];
      switch (Esc) {
        case '"': Out.push_back('"'); break;
        case '\\': Out.push_back('\\'); break;
        case '/': Out.push_back('/'); break;
        case 'n': Out.push_back('\n'); break;
        case 'r': Out.push_back('\r'); break;
        case 't': Out.push_back('\t'); break;
        case 'b': Out.push_back('\b'); break;
        case 'f': Out.push_back('\f'); break;
        default: Out.push_back(Esc); break;  // \uXXXX et al — not decoded
      }
      Cursor += 2;
      continue;
    }
    Out.push_back(C);
    ++Cursor;
  }
  return false;
}
