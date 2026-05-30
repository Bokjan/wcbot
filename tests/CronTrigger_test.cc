#include <gtest/gtest.h>

#include "Utility/CronTrigger.h"

using wcbot::CronTrigger;

TEST(CronTriggerSet, BitsAreIndependent) {
  CronTrigger Trig;
  Trig.SetMinute(0);
  Trig.SetMinute(30);
  EXPECT_TRUE(Trig.IsSetMinute(0));
  EXPECT_TRUE(Trig.IsSetMinute(30));
  EXPECT_FALSE(Trig.IsSetMinute(15));
}

TEST(CronTriggerEvery, EveryFlagSetsAllBitsForThatField) {
  CronTrigger Trig;
  Trig.SetMonth(CronTrigger::kEvery);
  for (int M = 1; M <= 12; ++M) {
    EXPECT_TRUE(Trig.IsSetMonth(M)) << "Month=" << M;
  }
}

TEST(CronTriggerOnly, IsOnlyFlagClearsThenSets) {
  CronTrigger Trig;
  Trig.SetHour(5);
  Trig.SetHour(10);
  ASSERT_TRUE(Trig.IsSetHour(5));
  ASSERT_TRUE(Trig.IsSetHour(10));

  // `kOnly` clears the *bit being set* before re-setting it; it does NOT
  // wipe the whole field. This matches the implementation comment / call
  // sites in TimeWheel which rely on additive Set semantics.
  Trig.SetHour(10, CronTrigger::kOnly);
  EXPECT_TRUE(Trig.IsSetHour(5));
  EXPECT_TRUE(Trig.IsSetHour(10));
}

TEST(CronTriggerClear, RemovesSingleBit) {
  CronTrigger Trig;
  Trig.SetDayOfMonth(1);
  Trig.SetDayOfMonth(15);
  Trig.ClearDayOfMonth(1);
  EXPECT_FALSE(Trig.IsSetDayOfMonth(1));
  EXPECT_TRUE(Trig.IsSetDayOfMonth(15));
}

TEST(CronTriggerSetAllEvery, EveryFieldFullyPopulated) {
  CronTrigger Trig;
  Trig.SetAllEvery();
  EXPECT_TRUE(Trig.IsSetMinute(0));
  EXPECT_TRUE(Trig.IsSetMinute(59));
  EXPECT_TRUE(Trig.IsSetHour(23));
  EXPECT_TRUE(Trig.IsSetMonth(12));
  EXPECT_TRUE(Trig.IsSetDayOfMonth(31));
  EXPECT_TRUE(Trig.IsSetDayOfWeek(7));
}

TEST(CronTriggerEquality, SameStateCompareEqual) {
  CronTrigger A;
  A.SetMonth(3);
  A.SetHour(9);
  CronTrigger B;
  B.SetMonth(3);
  B.SetHour(9);
  EXPECT_TRUE(A == B);
  EXPECT_FALSE(A < B);
  EXPECT_FALSE(B < A);
}

TEST(CronTriggerOrdering, DifferentStateProducesStrictOrder) {
  CronTrigger A;
  CronTrigger B;
  B.SetMinute(1);
  // Implementation uses memcmp; we don't pin the direction (it's an
  // opaque total order), only that exactly one of (A<B) / (B<A) is true.
  EXPECT_NE(A == B, true);
  EXPECT_TRUE((A < B) ^ (B < A));
}
