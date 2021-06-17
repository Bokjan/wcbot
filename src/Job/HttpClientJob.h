#pragma once

#include "Job.h"

namespace wcbot {

class HttpClientJob : public Job {
 public:
  explicit HttpClientJob(ThreadContext* Worker) : Job(Worker) {}
};

}  // namespace wcbot
