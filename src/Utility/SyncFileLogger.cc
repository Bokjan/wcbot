#include "SyncFileLogger.h"

#include <fcntl.h>
#include <unistd.h>

namespace wcbot {

SyncFileLogger::SyncFileLogger() : FD(0) {}

SyncFileLogger::~SyncFileLogger() { Close(); }

int SyncFileLogger::SetFile(const std::string &FileName) {
  std::lock_guard<std::mutex> Lock(Mutex);
  this->FileName = FileName;
  return Open();
}

int SyncFileLogger::Open() {
  if (FD > 0) {
    Close();
  }
  FD = open(FileName.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
  if (FD <= 0) {
    LOG_ERROR("SyncFileLogger::Open open=%d", FD);
    return FD;
  }
  return 0;
}

int SyncFileLogger::Close() {
  // not opened yet
  if (FD <= 0) {
    return 0;
  }
  int Ret = close(FD);
  if (Ret != 0) {
    LOG_ERROR("SyncFileLogger::Close close=%d", Ret);
    return Ret;
  }
  FD = 0;
  return 0;
}

void SyncFileLogger::Log(const char *Format, va_list Arguments) {
  std::lock_guard<std::mutex> Lock(Mutex);
  if (FD <= 0) {
    return;
  }
  char Buffer[64 * 1024];  // 64 KiB
  int Length = vsnprintf(Buffer, sizeof(Buffer), Format, Arguments);
  if (Length <= 0) {
    return;
  }
  // vsnprintf may report the would-be length when output is truncated;
  // clamp to the actual buffer size before writing.
  if (static_cast<size_t>(Length) > sizeof(Buffer)) {
    Length = static_cast<int>(sizeof(Buffer));
  }
  (void)write(FD, Buffer, Length);
}

}  // namespace wcbot
