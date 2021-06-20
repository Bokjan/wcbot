#include "CronTrigger.h"

#include <cstring>

namespace wcbot {

namespace cron_trigger_impl {

template <typename T>
inline T SetBit(T Value, int Position) {
  if (Position == ::wcbot::CronTrigger::kEvery) {
    return ~(static_cast<T>(0));
  }
  return Value | (static_cast<T>(1) << Position);
}

template <typename T>
inline T GetBit(T Value, int Position) {
  return Value & (static_cast<T>(1) << Position);
}

template <typename T>
inline T ClearBit(T Value, int Position) {
  return Value & (~(static_cast<T>(1) << Position));
}

}  // namespace cron_trigger_impl

CronTrigger::CronTrigger() : Minute(0), Hour(0), Month(0), DayOfWeek(0), DayOfMonth(0) {}

bool CronTrigger::operator<(const CronTrigger &Other) const {
  return memcmp(this, &Other, sizeof(*this)) < 0;
}

bool CronTrigger::operator==(const CronTrigger &Other) const {
  return memcmp(this, &Other, sizeof(*this)) == 0;
}

void CronTrigger::SetAllEvery() {
  Minute = kEvery;
  Hour = kEvery;
  Month = kEvery;
  DayOfWeek = kEvery;
  DayOfMonth = kEvery;
}

void CronTrigger::SetMinute(int Value, bool IsOnly) {
  if (IsOnly) {
    ClearMinute(Value);
  }
  Minute = cron_trigger_impl::SetBit(Minute, Value);
}

void CronTrigger::SetHour(int Value, bool IsOnly) {
  if (IsOnly) {
    ClearHour(Value);
  }
  Hour = cron_trigger_impl::SetBit(Hour, Value);
}

void CronTrigger::SetDayOfMonth(int Value, bool IsOnly) {
  if (IsOnly) {
    ClearDayOfMonth(Value);
  }
  DayOfMonth = cron_trigger_impl::SetBit(DayOfMonth, Value);
}

void CronTrigger::SetMonth(int Value, bool IsOnly) {
  if (IsOnly) {
    ClearMonth(Value);
  }
  Month = cron_trigger_impl::SetBit(Month, Value);
}

void CronTrigger::SetDayOfWeek(int Value, bool IsOnly) {
  if (IsOnly) {
    ClearDayOfWeek(Value);
  }
  DayOfWeek = cron_trigger_impl::SetBit(DayOfWeek, Value);
}

void CronTrigger::ClearMinute(int Value) { Minute = cron_trigger_impl::ClearBit(Minute, Value); }

void CronTrigger::ClearHour(int Value) { Hour = cron_trigger_impl::ClearBit(Hour, Value); }

void CronTrigger::ClearDayOfMonth(int Value) {
  DayOfMonth = cron_trigger_impl::ClearBit(DayOfMonth, Value);
}

void CronTrigger::ClearMonth(int Value) { Month = cron_trigger_impl::ClearBit(Month, Value); }

void CronTrigger::ClearDayOfWeek(int Value) {
  DayOfWeek = cron_trigger_impl::ClearBit(DayOfWeek, Value);
}

bool CronTrigger::IsSetMinute(int Value) const {
  return cron_trigger_impl::GetBit(Minute, Value) != 0;
}

bool CronTrigger::IsSetHour(int Value) const { return cron_trigger_impl::GetBit(Hour, Value) != 0; }

bool CronTrigger::IsSetDayOfMonth(int Value) const {
  return cron_trigger_impl::GetBit(DayOfMonth, Value) != 0;
}

bool CronTrigger::IsSetMonth(int Value) const {
  return cron_trigger_impl::GetBit(Month, Value) != 0;
}

bool CronTrigger::IsSetDayOfWeek(int Value) const {
  return cron_trigger_impl::GetBit(DayOfWeek, Value) != 0;
}

uint64_t CronTrigger::GetMinute() const { return Minute; }

uint32_t CronTrigger::GetHour() const { return Hour; }

uint32_t CronTrigger::GetDayOfMonth() const { return DayOfMonth; }

uint32_t CronTrigger::GetMonth() const { return Month; }

uint32_t CronTrigger::GetDayOfWeek() const { return DayOfWeek; }

}  // namespace wcbot
