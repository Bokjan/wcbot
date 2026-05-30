#pragma once

#include <string>

#include "../Job/Job.h"

namespace wcbot {

class WeComUploadJob final : public Job {
 public:
  WeComUploadJob();
  WeComUploadJob(const WeComUploadJob &) = delete;
  WeComUploadJob(const WeComUploadJob &&) = delete;

  Step OnStep(Job *Trigger) override;

  // Inputs (set before InvokeChild):
  const void *Data;
  uint64_t Length;
  std::string FileName;
  // Outputs (read in parent's OnStep(Trigger=this) before this object dies):
  std::string MediaId;
  int Code;
  std::string Msg;

  enum Error : int {
    kErrTooLarge = 1,
    kErrRspFailed = 2,
    kErrRspPkgInvalid = 3,
    kErrRspNoMediaId = 4,
  };

 private:
  enum class StateEnum { kUploadMediaReq, kUploadMediaRsp };
  StateEnum State;
};

}  // namespace wcbot