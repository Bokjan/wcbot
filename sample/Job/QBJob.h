#pragma once

#include "wcbot/Job/Job.h"

using wcbot::Job;

class QBJob final : public Job {
 public:
  explicit QBJob(): Job() { }
  QBJob(const QBJob&) = delete;
  QBJob(const QBJob&&) = delete;
  void Do(Job* Trigger = nullptr) override;
  void OnTimeout(Job* Trigger) override;
  void DeleteThis() { delete this; }
};
