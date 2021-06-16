#pragma once

#include <cstdio>
#include <string>

namespace wcbot {

class Logger {
 public:
  enum LogLevel : int {
    kUnknown = -1,
    kOff = 0,
    kTrace,
    kDebug,
    kInfo,
    kWarn,
    kError,
    kFatal,
    kAll
  };

  Logger() : CurrentLevel(kWarn) {}
  virtual ~Logger();
  virtual void Log(LogLevel Level, const char *Format, va_list Arguments) = 0;
  static const char *GetTimeCString();
  void Log(LogLevel Level, const char *Format, ...);
  bool SetLevel(const std::string &Target);
  bool SetLevel(LogLevel Target) {
    if (Target < kOff || Target > kAll) {
      return false;
    }
    CurrentLevel = Target;
    return true;
  }

 private:
  LogLevel CurrentLevel;
};

namespace logger_internal {
extern Logger *g_Logger;
extern const char *g_LogLevelCString[];
}  // namespace logger_internal

#define LOG_LOG_FORWARD(lvl, fmt, args...)                                 \
  wcbot::logger_internal::g_Logger->Log(                                   \
      lvl, "[%s] %s (%s:%d) " fmt "\n", ::wcbot::Logger::GetTimeCString(), \
      ::wcbot::logger_internal::g_LogLevelCString[lvl], __FUNCTION__, __LINE__, ##args)
#define LOG_TRACE(fmt, args...) LOG_LOG_FORWARD(::wcbot::Logger::kTrace, fmt, ##args)
#define LOG_DEBUG(fmt, args...) LOG_LOG_FORWARD(::wcbot::Logger::kDebug, fmt, ##args)
#define LOG_INFO(fmt, args...) LOG_LOG_FORWARD(::wcbot::Logger::kInfo, fmt, ##args)
#define LOG_WARN(fmt, args...) LOG_LOG_FORWARD(::wcbot::Logger::kWarn, fmt, ##args)
#define LOG_ERROR(fmt, args...) LOG_LOG_FORWARD(::wcbot::Logger::kError, fmt, ##args)
#define LOG_FATAL(fmt, args...) LOG_LOG_FORWARD(::wcbot::Logger::kFatal, fmt, ##args)
#define LOG_ALL(fmt, args...) LOG_LOG_FORWARD(::wcbot::Logger::kAll, fmt, ##args)

class StderrLogger final : public Logger {
 public:
  void Log(LogLevel Level, const char *Format, va_list Arguments);
};

namespace logger_internal {
extern StderrLogger DefaultStderrLogger;
}  // namespace logger_internal

}  // namespace wcbot
