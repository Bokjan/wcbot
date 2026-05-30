#pragma once

#include "wcbot/Job/Job.h"

class QBJob final : public wcbot::Job {
 public:
  QBJob() : wcbot::Job(), State(StateEnum::kInit) {}
  QBJob(const QBJob&) = delete;
  QBJob(const QBJob&&) = delete;

  Step OnStep(wcbot::Job* Trigger) override;

 private:
  enum class StateEnum { kInit, kWaitPush };
  StateEnum State;
};