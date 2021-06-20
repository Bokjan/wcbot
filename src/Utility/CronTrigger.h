#pragma once

#include <cstdint>

namespace wcbot {

class TimeWheel;
class CronTrigger final {
 public:
  static constexpr int kEvery = -1;
  static constexpr bool kOnly = true;

  CronTrigger();
  bool operator<(const CronTrigger &Other) const;
  bool operator==(const CronTrigger &Other) const;
  void SetAllEvery();
  void SetMinute(int, bool IsOnly = false);      // range: 0 ~ 59
  void SetHour(int, bool IsOnly = false);        // range: 0 ~ 23
  void SetDayOfMonth(int, bool IsOnly = false);  // range: 1 ~ 31
  void SetMonth(int, bool IsOnly = false);       // range: 1 ~ 12, different from `struct tm`
  void SetDayOfWeek(int, bool IsOnly = false);   // range: 1 ~ 7,  different from `struct tm`
  void ClearMinute(int);
  void ClearHour(int);
  void ClearDayOfMonth(int);
  void ClearMonth(int);
  void ClearDayOfWeek(int);
  bool IsSetMinute(int) const;
  bool IsSetHour(int) const;
  bool IsSetDayOfMonth(int) const;
  bool IsSetMonth(int) const;
  bool IsSetDayOfWeek(int) const;
  uint64_t GetMinute() const;
  uint32_t GetHour() const;
  uint32_t GetDayOfMonth() const;
  uint32_t GetMonth() const;
  uint32_t GetDayOfWeek() const;

 private:
  uint64_t Minute;
  uint32_t Hour;
  uint32_t Month;
  uint32_t DayOfWeek;
  uint32_t DayOfMonth;
  friend class TimeWheel;
};

}  // namespace wcbot
