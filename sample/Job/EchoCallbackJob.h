#pragma once

#include "wcbot/Job/MessageCallbackJob.h"

class EchoCallbackJob : public wcbot::MessageCallbackJob {
 public:
  Step OnStep(wcbot::Job* Trigger) override;
};