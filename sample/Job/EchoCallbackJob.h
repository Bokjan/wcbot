#pragma once

#include "wcbot/Job/MessageCallbackJob.h"

class EchoCallbackJob : public wcbot::MessageCallbackJob {
public:
  void Do(Job *Trigger = nullptr) override;
};
