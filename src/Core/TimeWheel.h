#pragma once

#include "../Job/Job.h"
#include "../Utility/CronTrigger.h"

namespace wcbot {

using FN_TimeWheelTickFunction = void (*)(FN_CreateJob, void *UserData);

class TimeWheelImpl;
class TimeWheel final {
 public:
  TimeWheel();
  ~TimeWheel();

  void AddCron(const CronTrigger &Trigger, FN_CreateJob Function);
  void Tick(FN_TimeWheelTickFunction Function, void *UserData);  // invoke this every minute!

 private:
  TimeWheelImpl *PImpl;
};

}  // namespace wcbot
