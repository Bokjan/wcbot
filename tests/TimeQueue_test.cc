#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "Core/TimeQueue.h"
#include "Job/Job.h"

using wcbot::DelayQueue;
using wcbot::IOJob;
using wcbot::Job;
using wcbot::SleepQueue;

namespace {

// Minimal IOJob that records OnStep visits but never touches the framework
// (no ArmTimeout/Sleep). We invoke `DelayQueue::Join` directly to inject the
// job into the queue without needing a worker thread context.
class TestIOJob : public IOJob {
 public:
  Step OnStep(Job * /*Trigger*/) override { return Step::kDone; }
};

class TestJob : public Job {
 public:
  Step OnStep(Job * /*Trigger*/) override { return Step::kDone; }
};

using SteadyClock = std::chrono::steady_clock;

}  // namespace

// -----------------------------------------------------------------------------
// DelayQueue
// -----------------------------------------------------------------------------

TEST(DelayQueue, EmptyDequeueReturnsNull) {
  DelayQueue Q;
  EXPECT_EQ(Q.Dequeue(SteadyClock::now()), nullptr);
}

TEST(DelayQueue, OrderedByTimeout) {
  DelayQueue Q;
  std::vector<TestIOJob *> Owned;
  // Insert in deliberately scrambled order. The queue is expected to dequeue
  // them in earliest-deadline-first order.
  for (int Ms : {300, 100, 200}) {
    auto *J = new TestIOJob();
    Q.Join(J, Ms);
    Owned.push_back(J);
  }
  // Skip enough wall time for the 100ms / 200ms / 300ms entries to be ready.
  std::this_thread::sleep_for(std::chrono::milliseconds(350));
  std::vector<int> Order;
  while (auto *J = Q.Dequeue(SteadyClock::now())) {
    // Map back to the join order via JobId. JobIds are 0,1,2 in insert order.
    Order.push_back(static_cast<int>(J->GetJobId()));
  }
  // Insert order was {300ms→Id0, 100ms→Id1, 200ms→Id2}.
  // Earliest first: Id1 (100ms), Id2 (200ms), Id0 (300ms).
  ASSERT_EQ(Order.size(), 3u);
  EXPECT_EQ(Order[0], 1);
  EXPECT_EQ(Order[1], 2);
  EXPECT_EQ(Order[2], 0);
  for (auto *J : Owned) delete J;
}

TEST(DelayQueue, RemoveByIdSkipsLaterDequeue) {
  DelayQueue Q;
  auto *A = new TestIOJob();
  auto *B = new TestIOJob();
  Q.Join(A, 50);
  Q.Join(B, 50);
  // Remove A explicitly (simulates "request finished before timeout fired").
  EXPECT_EQ(Q.Remove(A->GetJobId()), A);
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  // Only B should come out; A was removed.
  auto *First = Q.Dequeue(SteadyClock::now());
  ASSERT_EQ(First, B);
  EXPECT_EQ(Q.Dequeue(SteadyClock::now()), nullptr);
  delete A;
  delete B;
}

TEST(DelayQueue, RemoveOnEmptyReturnsNull) {
  DelayQueue Q;
  EXPECT_EQ(Q.Remove(42), nullptr);
}

// -----------------------------------------------------------------------------
// SleepQueue
// -----------------------------------------------------------------------------

TEST(SleepQueue, OrderedByWakeUp) {
  SleepQueue Q;
  std::vector<TestJob *> Owned;
  for (int Ms : {200, 50, 100}) {
    auto *J = new TestJob();
    Q.Join(J, Ms);
    Owned.push_back(J);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
  std::vector<TestJob *> Out;
  while (auto *J = Q.Dequeue(SteadyClock::now())) {
    Out.push_back(static_cast<TestJob *>(J));
  }
  ASSERT_EQ(Out.size(), 3u);
  // 50ms first, then 100ms, then 200ms. Compare via the original insert
  // order: index 1 (50), index 2 (100), index 0 (200).
  EXPECT_EQ(Out[0], Owned[1]);
  EXPECT_EQ(Out[1], Owned[2]);
  EXPECT_EQ(Out[2], Owned[0]);
  for (auto *J : Owned) delete J;
}

TEST(SleepQueue, NotYetExpired) {
  SleepQueue Q;
  auto *J = new TestJob();
  Q.Join(J, 10000);  // far future
  EXPECT_EQ(Q.Dequeue(SteadyClock::now()), nullptr);
  delete J;
}
