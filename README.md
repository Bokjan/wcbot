# wcbot
A brand new Group Bot (群机器人) library for WeCom (企业微信). Former version is [DiaoBot](https://github.com/Bokjan/DiaoBot). Tested on macOS 11.3.1, Ubuntu 20.04, and tlinux 2.2. 

||wcbot|[DiaoBot](https://github.com/Bokjan/DiaoBot)|
--|--|--
Platform|\*nix|Linux
Library type|Static|Dynamic (SDK)
Deployment|Single executable|Exe + SDK dylib + user dylib
Config file|JSON|XML-like TFC config
I/O multiplexing|[libuv](https://github.com/libuv/libuv)|`select()` by [mongoose](https://github.com/cesanta/mongoose)
Parallel model|Multi-threading|Multi-threading
Thread model|1 main, N worker|1 acceptor, 1 cronjob, N worker
Worker model|Asynchronous (state machine)|Synchronous, thread-blocking
Performance|High|Low
Extensibility|High|N/A

This time, we develop `wcbot` from scratch. Performance and R&D efficiency are improved a lot. 

# Prerequisites
- CMake 3.1 +
- C++ compiler with C++11 support
- `libuv`
- `libcurl`
- `OpenSSL` (`libcrypto`)
- `RapidJSON`

Other 3rd party libraries are distributed as `git submodule`s.

# Build
Just `cmake` it. 

# Run
The final artifact is a executable (built by yourself). Assume the executable is `wcbotd`, and there's a config file named `config.json` in the same directory. 

Run as daemon:
```bash
./wcbotd config.json
```
Don't want a daemon? Add any character(s) as the 3rd command line argument, like: 
```bash
./wcbotd config.json nofork
```