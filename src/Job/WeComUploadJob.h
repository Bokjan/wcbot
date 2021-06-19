#pragma once

#include <string>

#include "Job.h"

namespace wcbot {

class WeComUploadJob final : public Job {
 public:
  explicit WeComUploadJob(Job *Receiver);
  WeComUploadJob(const WeComUploadJob &) = delete;
  WeComUploadJob(const WeComUploadJob &&) = delete;
  void Do(Job *Trigger = nullptr);
  void OnTimeout(Job *Trigger);
  void DeleteThis() { delete this; }

  const void *Data;       // IN
  uint64_t Length;        // IN
  std::string FileName;   // IN
  std::string MediaId;    // OUT
  int Code;               // OUT
  std::string Msg;        // OUT

  enum Error : int {
    kErrTooLarge = -1,
    kRspFailed = -2,
    kRspPkgInvalid = -3,
    kRspNoMediaId = -4
  };

 private:
  enum class StateEnum { kUploadMediaReq, kUploadMediaRsp, kError };
  StateEnum State;

  void DoUploadMediaReq();
  void DoUploadMediaRsp(Job *Rsp);
  void DoError();
};

}  // namespace wcbot
