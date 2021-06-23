#pragma once

#include "Logger.h"

#include <mutex>

namespace wcbot {

class SyncFileLogger : public Logger {
 public:
  SyncFileLogger();
  ~SyncFileLogger();
  void Log(const char *Format, va_list Arguments);
  int SetFile(const std::string &FileName);

 protected:
  int FD;
  std::string FileName;
  std::mutex Mutex;
  int Open();
  int Close();
};

}  // namespace wcbot
