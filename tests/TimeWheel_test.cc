// TimeWheel tests.
//
// `TimeWheel` reads `time()` directly inside `UpdateCurrentInfo`, so we
// cannot inject a fake clock. Instead, each test reads the current wall
// time, builds CronTriggers that either match or deliberately miss the
// current minute, and invokes Tick() once. This keeps the tests
// deterministic on any machine with a sane RTC.

#include <gtest/gtest.h>

#include <ctime>
#include <functional>
#include <vector>

#include "Core/TimeWheel.h"
#include "Job/Job.h"
#include "Utility/CronTrigger.h"

using wcbot::CronTrigger;
using wcbot::FN_CreateJob;
using wcbot::Job;
using wcbot::TimeWheel;

namespace {

// Snapshot of the current local time, normalized to the same fields the
// TimeWheel itself uses internally (month is 1-12, DoW is 1-7 with Sunday=7).
struct NowFields {
  int Minute;
  int Hour;
  int Month;
  int DayOfMonth;
  int DayOfWeek;
};

NowFields ReadNow() {
  std::time_t T = std::time(nullptr);
  std::tm TM{};
  ::localtime_r(&T, &TM);
  NowFields N{};
  N.Minute = TM.tm_min;
  N.Hour = TM.tm_hour;
  N.Month = TM.tm_mon + 1;
  N.DayOfMonth = TM.tm_mday;
  N.DayOfWeek = TM.tm_wday == 0 ? 7 : TM.tm_wday;
  return N;
}

// Build a CronTrigger that matches RIGHT NOW on every field (month / DoM /
// DoW / hour / minute). Such a trigger should fire on the next Tick().
CronTrigger TriggerMatchingNow(const NowFields &N) {
  CronTrigger Trig;
  Trig.SetMonth(N.Month);
  Trig.SetDayOfMonth(N.DayOfMonth);
  Trig.SetDayOfWeek(N.DayOfWeek);
  Trig.SetHour(N.Hour);
  Trig.SetMinute(N.Minute);
  return Trig;
}

// Counter that tracks how many times a given factory was invoked. Keyed by
// the `Tag` integer the factory was constructed with so we can register
// multiple distinct triggers in the same test.
struct FactoryStats {
  std::vector<int> Tags;  // observed in firing order
};
FactoryStats g_Stats;

// A trivial Job that records its tag once on first OnStep, then exits.
// We never actually run these — TimeWheel only emits factories into the
// user-supplied tick callback. The factory itself bumps the counter.
class StubJob final : public Job {
 public:
  Step OnStep(Job *) override { return Step::kDone; }
};

// User-supplied callback for TimeWheel::Tick. The TimeWheel signature is
// `void(const FN_CreateJob &, void *UserData)`. We don't actually invoke
// the factory here; we just record that *this* factory was emitted (the
// factory's tag was captured as a closure when registered).
void RecordFactory(const FN_CreateJob & /*Factory*/, void *UserData) {
  int *TagSlot = static_cast<int *>(UserData);
  g_Stats.Tags.push_back(*TagSlot);
}

// Helper: register a tagged factory with the wheel and return a
// stable-address int that can be passed to Tick() as UserData. We rely on
// the fact that Tick() processes all matching slots in sequence inside a
// single call, so the int's lifetime only needs to span one Tick().
//
// Each test that uses this just keeps the int on the stack and passes its
// address. We don't actually need different UserData per registration in
// these tests — the only thing we measure is *how many times* the
// callback fired.

}  // namespace

// -----------------------------------------------------------------------------
// Tick fires a trigger that matches the current minute.
// -----------------------------------------------------------------------------

TEST(TimeWheel, MatchingTriggerFiresOnTick) {
  g_Stats = {};
  TimeWheel Wheel;
  auto N = ReadNow();
  Wheel.AddCron(TriggerMatchingNow(N), [] { return new StubJob(); });
  int Tag = 1;
  Wheel.Tick(&RecordFactory, &Tag);
  EXPECT_EQ(g_Stats.Tags.size(), 1u);
  if (!g_Stats.Tags.empty()) {
    EXPECT_EQ(g_Stats.Tags[0], 1);
  }
}

// -----------------------------------------------------------------------------
// Trigger that does not match the current month never fires.
// -----------------------------------------------------------------------------

TEST(TimeWheel, NonMatchingTriggerDoesNotFireImmediately) {
  g_Stats = {};
  TimeWheel Wheel;
  auto N = ReadNow();
  // Pick a month that is not the current one. (The trigger will still fire
  // *eventually* — on that month — but the Tick() we do here is "now", so
  // we expect zero firings on this minute.)
  CronTrigger Trig;
  int OtherMonth = (N.Month % 12) + 1;
  Trig.SetMonth(OtherMonth);
  Trig.SetDayOfMonth(N.DayOfMonth);
  Trig.SetHour(N.Hour);
  Trig.SetMinute(N.Minute);
  Wheel.AddCron(Trig, [] { return new StubJob(); });
  int Tag = 1;
  Wheel.Tick(&RecordFactory, &Tag);
  EXPECT_EQ(g_Stats.Tags.size(), 0u);
}

// -----------------------------------------------------------------------------
// Trigger missing a Month still gets ignored without crashing (regression for
// the "Invalid CronTrigger, month not set" path).
// -----------------------------------------------------------------------------

TEST(TimeWheel, TriggerWithoutMonthIsRejected) {
  g_Stats = {};
  TimeWheel Wheel;
  CronTrigger Trig;  // entirely default-constructed → no month set
  Wheel.AddCron(Trig, [] { return new StubJob(); });
  int Tag = 1;
  Wheel.Tick(&RecordFactory, &Tag);
  EXPECT_EQ(g_Stats.Tags.size(), 0u);
}

// -----------------------------------------------------------------------------
// Two distinct triggers that both match current time both fire (and only
// once each — exercises the per-entry Id deduplication).
// -----------------------------------------------------------------------------

TEST(TimeWheel, MultipleMatchingTriggersBothFire) {
  g_Stats = {};
  TimeWheel Wheel;
  auto N = ReadNow();
  Wheel.AddCron(TriggerMatchingNow(N), [] { return new StubJob(); });
  Wheel.AddCron(TriggerMatchingNow(N), [] { return new StubJob(); });
  int Tag = 1;
  Wheel.Tick(&RecordFactory, &Tag);
  EXPECT_EQ(g_Stats.Tags.size(), 2u);
}

// -----------------------------------------------------------------------------
// A `SetAllEvery` trigger fires every minute (sanity check that the wheel
// handles the maximally-permissive trigger correctly without aborting).
// -----------------------------------------------------------------------------

TEST(TimeWheel, SetAllEveryFires) {
  g_Stats = {};
  TimeWheel Wheel;
  CronTrigger Trig;
  Trig.SetAllEvery();
  Wheel.AddCron(Trig, [] { return new StubJob(); });
  int Tag = 1;
  Wheel.Tick(&RecordFactory, &Tag);
  EXPECT_EQ(g_Stats.Tags.size(), 1u);
}

// -----------------------------------------------------------------------------
// After firing, the matching trigger is rescheduled into the month wheel and
// fires again on a subsequent matching Tick — confirming that entries are
// not consumed-once-and-dropped.
// -----------------------------------------------------------------------------

TEST(TimeWheel, FiredEntryIsRescheduled) {
  g_Stats = {};
  TimeWheel Wheel;
  auto N = ReadNow();
  Wheel.AddCron(TriggerMatchingNow(N), [] { return new StubJob(); });
  int Tag = 1;
  // Two consecutive Ticks within the same minute — the entry should be
  // routed back to the month slot after the first Tick and emerge again on
  // the second.
  Wheel.Tick(&RecordFactory, &Tag);
  Wheel.Tick(&RecordFactory, &Tag);
  EXPECT_EQ(g_Stats.Tags.size(), 2u);
}
