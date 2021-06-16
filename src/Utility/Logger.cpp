#include "Logger.h"

#include <sys/time.h>
#include <cstdarg>
#include <ctime>
#include <map>

namespace wcbot {

namespace logger_internal {
StderrLogger DefaultStderrLogger;
Logger *g_Logger = &logger_internal::DefaultStderrLogger;
const char *g_LogLevelCString[] = {"<OFF>  ", "<TRACE>", "<DEBUG>", "<INFO> ",
                                   "<WARN> ", "<ERROR>", "<FATAL>", "<ALL>  "};
}  // namespace logger_internal

Logger::~Logger() {}

void Logger::Log(LogLevel Level, const char *Format, ...) {
  if (Level < CurrentLevel) {
    return;
  }
  va_list Arguments;
  va_start(Arguments, Format);
  this->Log(Level, Format, Arguments);
  va_end(Arguments);
}

const char *Logger::GetTimeCString() {
  constexpr ssize_t kBufferLen = 128;
  thread_local char Buffer[kBufferLen];
  struct timeval TimeVal;
  gettimeofday(&TimeVal, nullptr);
  struct tm *TM = localtime(&TimeVal.tv_sec);
  snprintf(Buffer, sizeof(Buffer), "%04d%02d%02d %02d:%02d:%02d.%.6ld", 1900 + TM->tm_year,
           1 + TM->tm_mon, TM->tm_mday, TM->tm_hour, TM->tm_min, TM->tm_sec, TimeVal.tv_usec);
  return Buffer;
}

bool Logger::SetLevel(const std::string &Target) {
  static std::map<std::string, LogLevel> StrEnumMap = {
      {"off", kOff},   {"trace", kTrace}, {"debug", kDebug}, {"info", kInfo},
      {"warn", kWarn}, {"error", kError}, {"fatal", kFatal}, {"all", kAll},
  };
  auto Finder = StrEnumMap.find(Target);
  if (Finder == StrEnumMap.end()) {
    return false;
  }
  this->SetLevel(Finder->second);
  return true;
}

void StderrLogger::Log(LogLevel Level, const char *Format, va_list Arguments) {
  vfprintf(stderr, Format, Arguments);
}

}  // namespace wcbot
