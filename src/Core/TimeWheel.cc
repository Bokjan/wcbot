#include "TimeWheel.h"

#include <ctime>  // C++11 has no calendar support in <chrono>

#include <list>

#include "../Job/Job.h"
#include "../Utility/Logger.h"

namespace wcbot {

static constexpr size_t kSlotSizeMinute = 60;
static constexpr size_t kSlotSizeHour = 24;
static constexpr size_t kSlotSizeDayOfWeek = 7 + 1;
static constexpr size_t kSlotSizeDayOfMonth = 31 + 1;
static constexpr size_t kSlotSizeMonth = 12 + 1;

class TimeWheelImpl {
 public:
  struct SlotElement {
    CronTrigger Trigger;
    FN_CreateJob Function;
    bool operator<(const SlotElement &Other) const {
      if (Function == Other.Function) {
        return Trigger < Other.Trigger;
      }
      return Function < Other.Function;
    }
    bool operator==(const SlotElement &Other) const {
      return Function == Other.Function && Trigger == Other.Trigger;
    }
  };

  int CurrentMinute;
  int CurrentHour;
  int CurrentDayOfWeek;
  int CurrentDayOfMonth;
  int CurrentMonth;

  std::list<SlotElement> SlotMinute[kSlotSizeMinute];
  std::list<SlotElement> SlotHour[kSlotSizeHour];
  std::list<SlotElement> SlotDayOfWeek[kSlotSizeDayOfWeek];
  std::list<SlotElement> SlotDayOfMonth[kSlotSizeDayOfMonth];
  std::list<SlotElement> SlotMonth[kSlotSizeMonth];

  void UpdateCurrentInfo();
  void AddCron(const CronTrigger &Trigger, FN_CreateJob Function);
  void TickMinute(FN_TimeWheelTickFunction Function, void *UserData);
};

TimeWheel::TimeWheel() : PImpl(new TimeWheelImpl()) {}

TimeWheel::~TimeWheel() { delete PImpl; }

void TimeWheelImpl::UpdateCurrentInfo() {
  time_t T = time(nullptr);
  auto TM = localtime(&T);
  CurrentMinute = TM->tm_min;
  CurrentHour = TM->tm_hour;
  CurrentDayOfWeek = TM->tm_wday + 1;
  CurrentDayOfMonth = TM->tm_mday;
  CurrentMonth = TM->tm_mon + 1;
}

void TimeWheel::AddCron(const CronTrigger &Trigger, FN_CreateJob Function) {
  PImpl->AddCron(Trigger, Function);
}

void TimeWheelImpl::AddCron(const CronTrigger &Trigger, FN_CreateJob Function) {
  // update first
  UpdateCurrentInfo();
  // actually, add it to a month-level list
  int Target = -1;
  for (int i = CurrentMonth; i <= 12; ++i) {
    if (Trigger.IsSetMonth(i)) {
      Target = i;
      break;
    }
  }
  if (Target == -1) {
    for (int i = 1; i < CurrentMonth; ++i) {
      if (Trigger.IsSetMonth(i)) {
        Target = i;
        break;
      }
    }
  }
  if (Target == -1) {
    LOG_WARN("%s", "Invalid CronTrigger, month not set");
    return;
  }
  // add
  SlotMonth[Target].emplace_back((SlotElement){Trigger, Function});
}

void TimeWheel::Tick(FN_TimeWheelTickFunction Function, void *UserData) {
  PImpl->TickMinute(Function, UserData);
}

void TimeWheelImpl::TickMinute(FN_TimeWheelTickFunction Function, void *UserData) {
  LOG_TRACE("%s", "TimeWheelImpl::TickMinute");
  // update first
  UpdateCurrentInfo();
  // month
  std::list<SlotElement>::iterator ListIt;
  auto &MonthList = SlotMonth[CurrentMonth];
  for (auto ListIt = MonthList.begin(); ListIt != MonthList.end(); ++ListIt) {
    int TargetDoW = -1;
    int TargetDoM = -1;
    for (int i = CurrentDayOfWeek; i <= 7; ++i) {
      if (ListIt->Trigger.IsSetDayOfWeek(i)) {
        TargetDoW = i;
        break;
      }
    }
    if (TargetDoW == -1) {
      for (int i = 0; i < CurrentDayOfWeek; ++i) {
        if (ListIt->Trigger.IsSetDayOfWeek(i)) {
          TargetDoW = i;
          break;
        }
      }
    }
    for (int i = CurrentDayOfMonth; i <= 31; ++i) {
      if (ListIt->Trigger.IsSetDayOfMonth(i)) {
        TargetDoM = i;
        break;
      }
    }
    if (TargetDoM == -1) {
      for (int i = 0; i < CurrentDayOfMonth; ++i) {
        if (ListIt->Trigger.IsSetDayOfMonth(i)) {
          TargetDoM = i;
          break;
        }
      }
    }
    if (TargetDoM == -1 && TargetDoM == -1) {
      LOG_WARN("%s", "Invalid CronTrigger, DoM/DoW not set");
      continue;
    }
    // dispatch both, we'll de-duplicate on minute-level
    if (TargetDoW != -1) {
      SlotDayOfWeek[TargetDoW].emplace_back(*ListIt);
    }
    if (TargetDoM != -1) {
      SlotDayOfMonth[TargetDoM].emplace_back(*ListIt);
    }
  }
  MonthList.clear();
  // day of week
  auto &DayOfWeekList = SlotDayOfWeek[CurrentDayOfWeek];
  for (ListIt = DayOfWeekList.begin(); ListIt != DayOfWeekList.end(); /* empty */) {
    int Target = -1;
    for (int i = CurrentHour; i <= 23; ++i) {
      if (ListIt->Trigger.IsSetHour(i)) {
        Target = i;
        break;
      }
    }
    if (Target == -1) {
      for (int i = 0; i < CurrentHour; ++i) {
        if (ListIt->Trigger.IsSetHour(i)) {
          Target = i;
          break;
        }
      }
    }
    auto SpliceIt = ListIt++;
    if (Target == -1) {
      LOG_WARN("%s", "Invalid CronTrigger, hour not set");
      DayOfWeekList.erase(SpliceIt);
      continue;
    }
    SlotHour[Target].splice(SlotHour[Target].end(), DayOfWeekList, SpliceIt);
  }
  // day of month
  auto &DayOfMonthList = SlotDayOfMonth[CurrentDayOfMonth];
  for (ListIt = DayOfMonthList.begin(); ListIt != DayOfMonthList.end(); /* empty */) {
    int Target = -1;
    for (int i = CurrentHour; i < 24; ++i) {
      if (ListIt->Trigger.IsSetHour(i)) {
        Target = i;
        break;
      }
    }
    if (Target == -1) {
      for (int i = 0; i < CurrentHour; ++i) {
        if (ListIt->Trigger.IsSetHour(i)) {
          Target = i;
          break;
        }
      }
    }
    auto SpliceIt = ListIt++;
    if (Target == -1) {
      LOG_WARN("%s", "Invalid CronTrigger, hour not set");
      DayOfMonthList.erase(SpliceIt);
      continue;
    }
    SlotHour[Target].splice(SlotHour[Target].end(), DayOfMonthList, SpliceIt);
  }
  // hour
  auto &HourList = SlotHour[CurrentHour];
  for (ListIt = HourList.begin(); ListIt != HourList.end(); /* empty */) {
    int Target = -1;
    for (int i = CurrentMinute; i < 60; ++i) {
      if (ListIt->Trigger.IsSetMinute(i)) {
        Target = i;
        break;
      }
    }
    if (Target == -1) {
      for (int i = 0; i < CurrentMinute; ++i) {
        if (ListIt->Trigger.IsSetMinute(i)) {
          Target = i;
          break;
        }
      }
    }
    auto SpliceIt = ListIt++;
    if (Target == -1) {
      LOG_WARN("%s", "Invalid CronTrigger, hour not set");
      HourList.erase(SpliceIt);
      continue;
    }
    SlotMinute[Target].splice(SlotMinute[Target].end(), HourList, SpliceIt);
  }
  // minute
  auto &MinuteList = SlotMinute[CurrentMinute];
  MinuteList.sort();
  MinuteList.unique();
  for (ListIt = MinuteList.begin(); ListIt != MinuteList.end(); /* empty */) {
    // fire
    Function(ListIt->Function, UserData);
    // reset to month
    int Target = -1;
    for (int i = CurrentMonth; i <= 12; ++i) {
      if (ListIt->Trigger.IsSetMonth(i)) {
        Target = i;
        break;
      }
    }
    if (Target == -1) {
      for (int i = 0; i < CurrentMonth; ++i) {
        if (ListIt->Trigger.IsSetMonth(i)) {
          Target = i;
          break;
        }
      }
    }
    // guard
    auto SpliceIt = ListIt++;
    if (Target == -1) {
      LOG_ERROR("%s", "GUARD FAILURE");
      MinuteList.erase(SpliceIt);
      continue;
    }
    SlotMonth[Target].splice(SlotMonth[Target].end(), MinuteList, SpliceIt);
  }
}

}  // namespace wcbot
