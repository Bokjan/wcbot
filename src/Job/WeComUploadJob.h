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

  const void *Data;       // IN
  uint64_t Length;        // IN
  std::string FileName;   // IN
  std::string MediaId;    // OUT
  int Code;               // OUT
  std::string Msg;        // OUT

  enum Error : int {
    kErrTooLarge = 1,
    kErrRspFailed = 2,
    kErrRspPkgInvalid = 3,
    kErrRspNoMediaId = 4
  };

 private:
  enum class StateEnum { kUploadMediaReq, kUploadMediaRsp, kError };
  StateEnum State;

  void DoUploadMediaReq();
  void DoUploadMediaRsp(Job *Rsp);
  void DoError();
};

}  // namespace wcbot
