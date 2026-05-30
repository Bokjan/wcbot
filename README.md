# wcbot

A Group Bot (群机器人) library for WeCom (企业微信), written in C++11.
Predecessor: [DiaoBot](https://github.com/Bokjan/DiaoBot).

|              | wcbot                                  | DiaoBot                          |
|--------------|----------------------------------------|----------------------------------|
| `tlinux` native       | Yes                            | Yes                              |
| Library type          | Static                         | Dynamic (SDK)                    |
| Deployment            | Single executable              | Exe + SDK dylib + user dylib     |
| Config file           | JSON                           | XML-like TFC                     |
| I/O multiplexing      | [libuv](https://github.com/libuv/libuv) | `select()` (mongoose)   |
| Parallel model        | Multi-threading                | Multi-threading                  |
| Thread model          | 1 main + N worker              | 1 acceptor + 1 cronjob + N worker|
| Worker model          | Asynchronous (state machine)   | Synchronous, thread-blocking     |

## Architecture

- **Main thread**: TCP accept + dispatch, signal handling, cron timer.
- **Worker threads**: business logic via the **Job state machine** (no
  blocking syscalls inside a job). Each worker has its own `uv_loop_t`,
  delay/sleep queues, and a `curl_multi` handle driven by libuv.
- **Inter-Thread Communication (ITC)**: lock-protected queues + `uv_async_t`
  notifications. Three event types: `TcpMainToWorker`, `TcpWorkerToMain`,
  and `JobCreateAndRun`.

```
TCP accept  ──► [main]  ServerCodec.IsComplete?  ──► ITC ──► [worker N]
                                                              │
                                                              ▼
                                          HttpHandlerJob state machine
                                                              │
                              ITC (TcpWorkerToMain) ◄─────────┘
                                          │
                              [main] uv_write back to client
```

### Module layout (`src/`)

| Module      | Responsibility                                                  |
|-------------|-----------------------------------------------------------------|
| `Core/`     | Engine, worker thread, ITC, time wheel (cron), delay/sleep queue|
| `Job/`      | Async state-machine jobs (HTTP server/client, callback, upload) |
| `Codec/`    | Protocol codecs (HTTP request)                                  |
| `WeCom/`    | WeCom server-/client-message models                             |
| `Utility/`  | Logger, MemoryBuffer, CronTrigger, HttpPackage, common helpers  |
| `ThirdParty/`| `tinyxml2`, `WXBizMsgCrypt` (vendored)                         |

## Prerequisites

- CMake 3.10+ (tested up to CMake 3.30 via the `<min>...<max>` form)
- C++ compiler with C++17 support (gcc 7+ / clang 5+)
- `libuv`
- `libcurl`
- `OpenSSL` (`libcrypto` + `libssl`)
- `RapidJSON`

Sub-modules: `tinyxml2`, `WXBizMsgCrypt` (vendored under `src/ThirdParty/`).

## Build

```bash
mkdir -p build && cd build
cmake ..
cmake --build . -j
```

Useful options:

- `-DCMAKE_BUILD_TYPE=Debug` — debug build (also defines `DEBUG`).
- `-DWCBOT_BUILD_SAMPLE=OFF` — skip the sample executable.

## Run

The artifact is an executable you build yourself. Assuming the binary is
`wcbotd` and `config.json` sits next to it:

```bash
# Run as a daemon (default)
./wcbotd config.json

# Run in the foreground (any 3rd argument disables daemonization)
./wcbotd config.json nofork
```

## Configuration

`config.json` is a single JSON object with the following sections.

| Path                          | Type    | Description                                          |
|-------------------------------|---------|------------------------------------------------------|
| `Bot.WebHookKey`              | string  | WeCom webhook key (UUID).                            |
| `Bot.WebHookPrefix`           | string  | Webhook URL prefix, e.g. `https://qyapi.weixin.qq.com/cgi-bin/webhook`. |
| `Bot.Token`                   | string  | Callback token (used by `WXBizMsgCrypt`).            |
| `Bot.EncodingAesKey`          | string  | Callback EncodingAESKey (43 chars).                  |
| `Bot.CallbackPath`            | string  | HTTP path that handles WeCom callbacks, e.g. `/verify`. |
| `Http.BindIpv4`               | string  | Listen IP, e.g. `0.0.0.0`.                           |
| `Http.BindPort`               | int     | Listen port.                                         |
| `Network.MaxRecvBuffLength`   | uint64  | Max per-connection receive buffer (bytes).           |
| `Network.MaxSendBuffLength`   | uint64  | Max per-connection send buffer (bytes).              |
| `Framework.WorkerThread`      | uint32  | Number of worker threads.                            |
| `Log.LogLevel`                | string  | One of `trace/debug/info/warn/error/fatal/all/off`. |
| `Log.Type`                    | string  | `stderr` (default) or `sync_file`.                   |
| `Log.FilePath`                | string  | Log file path when `Log.Type == "sync_file"`.        |
| `CustomConfig`                | string  | Optional path to user-defined config.                |

## Usage

A minimal entry point looks like:

```cpp
#include "wcbot/Core/Engine.h"

int main(int argc, char* argv[]) {
  auto& Engine = wcbot::Engine::Get();
  if (!Engine.ParseArguments(argc, argv)) return 1;
  if (!Engine.Initialize())               return 1;

  Engine.RegisterCallbackHandler([] -> wcbot::MessageCallbackJob* {
      return new MyCallbackJob();
  });
  return Engine.Run();
}
```

A custom `Job` is a state machine driven by the framework via `OnStep`:

```cpp
class MyJob final : public wcbot::Job {
 public:
  Step OnStep(wcbot::Job* Trigger) override {
    switch (State) {
      case kStart:
        InvokeChild(new wcbot::HttpClientJob{ /* ... */ });
        State = kAwait;
        return Step::kWaiting;          // suspend until child finishes
      case kAwait:
        if (auto* C = AsJob<wcbot::HttpClientJob>(Trigger)) {
          // Read C->Response here — the child will be deleted right after
          // OnStep returns; capture anything you still need.
        }
        return Step::kDone;             // framework deletes us, notifies parent
    }
    return Step::kDone;
  }
  void OnCancel() override { /* release external resources if any */ }
};
```

Key contract points:

- The framework owns lifetime: `delete this` happens exactly once, in the
  framework, after `OnStep` returns `Step::kDone`.
- `Trigger` is `this` for self-driven steps (initial activation, sleep
  wakeup, armed-timeout fired) or a child `Job*` after the child finishes.
- For IO with timeout, derive from `wcbot::IOJob` and call `ArmTimeout(ms)`
  before returning `Step::kWaiting`. On timeout the framework sets
  `ErrCode = kErrTimeout` and re-enters `OnStep(this)`.
- When a job is cancelled (parent finished early or `Cancel()` called),
  `OnCancel` runs so the job can release external resources (e.g. detach a
  cURL handle); the framework deletes the job right after.

See `sample/` for full examples (`QBJob` for cron, `EchoCallbackJob` for
inbound message handling).

## Recent improvements

See [`docs/REFACTORING.md`](docs/REFACTORING.md) for a detailed list of
correctness, thread-safety, and build-system fixes applied in this round.
