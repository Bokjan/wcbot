#pragma once

#include <cstdint>

#include <functional>
#include <type_traits>
#include <vector>

namespace wcbot {

class Job;
namespace job_impl {
class Driver;
}  // namespace job_impl

// -----------------------------------------------------------------------------
// Job state machine — v2 protocol
// -----------------------------------------------------------------------------
//
// A `Job` represents one logical asynchronous task running inside a worker
// thread. The state machine is driven exclusively by the framework via
// `Job::Run`; the only entry point business code implements is `OnStep`.
//
//   * `Step::kContinue` — advance immediately; the framework re-enters
//                         `OnStep` on the same call stack.
//   * `Step::kWaiting`  — block on an external event (IO completion, child
//                         completion, sleep, or armed timeout); the framework
//                         returns control and will re-enter `OnStep` once the
//                         event arrives.
//   * `Step::kDone`     — business is finished; the framework cancels any
//                         still-running children, removes this job from its
//                         parent's child list, deletes this object, and
//                         finally re-drives the parent.
//
// The framework guarantees:
//   1. `delete this` happens exactly once, in the framework. Business code
//      never calls `delete` on a Job and never calls a "DeleteThis" helper.
//   2. When the parent is observed in `OnStep(Trigger=child)`, the child is
//      still alive; the parent must extract whatever it needs from the child
//      (e.g. via `TakeResponse()` style move accessors) before returning, as
//      the framework will delete the child immediately after the call.
//   3. When a job is cancelled (parent finishes / `Cancel()` called), the
//      framework invokes `OnCancel` so the job can release external resources
//      (e.g. detach a cURL easy handle) and then deletes the job.
//
// The `Trigger` argument to `OnStep` is one of:
//   * `this`            — self-driven step (initial activation, sleep wakeup,
//                         armed-timeout fired).
//   * a child `Job*`    — the named child has just completed; you may read
//                         its result-bearing members in this call only.
//   * `nullptr`         — a child has completed but its identity is not
//                         needed (rare; reserved for future use).
//
// -----------------------------------------------------------------------------

class Job {
 public:
  enum class Step : int {
    kContinue,  // advance to next state immediately
    kWaiting,   // wait for an external event
    kDone,      // business done; framework reclaims this job
  };

  enum ErrEnum { kErrTimeout = -9999 };

  Job();
  Job(const Job &) = delete;
  Job(Job &&) = delete;
  Job &operator=(const Job &) = delete;
  Job &operator=(Job &&) = delete;
  virtual ~Job();

  // Implement this. See file-level comment for the contract.
  virtual Step OnStep(Job *Trigger) = 0;

  // Override to release external resources when the framework reclaims this
  // job before its natural completion (e.g. parent finished early). The
  // default implementation is a no-op — the framework deletes this object
  // right after `OnCancel` returns.
  virtual void OnCancel() {}

  // Spawn a child job. Ownership is transferred to the framework; the child
  // is driven on the same worker thread. The child's first `OnStep` will
  // observe `Trigger == child_self`.
  //
  // Note: callers idiomatically pass `new ChildJob(...)`; the bare `new` is
  // intentional — it expresses "the framework now owns this object" and
  // mirrors the `delete` that happens inside the framework on kDone /
  // OnCancel. Using a `unique_ptr<Job>` here would not improve safety
  // because the wrapper would have to be released right back into a raw
  // pointer at the call boundary.
  void InvokeChild(Job *Child);

  // Sleep for `Millisecond` and then re-enter `OnStep(this)`. Must be paired
  // with `return Step::kWaiting`.
  void Sleep(int Millisecond);

  // Cooperatively cancel this job: framework will invoke `OnCancel` on this
  // job (and recursively on its children) and then delete it. Safe to call
  // from inside `OnStep` — the cancellation takes effect after the current
  // `OnStep` returns.
  void Cancel();

  // ErrCode = 0   : success
  // ErrCode < 0   : framework-defined error (see `ErrEnum`)
  // ErrCode > 0   : user-defined error
  int ErrCode;

 protected:
  // Helper: type-safely cast a `Trigger` pointer to a derived job type.
  template <class T>
  static T *AsJob(Job *J) {
    static_assert(std::is_base_of<Job, T>::value, "T must derive from Job");
    return dynamic_cast<T *>(J);
  }

 private:
  friend class job_impl::Driver;

  // Linkage maintained by the framework only.
  Job *Parent;
  std::vector<Job *> Children;
  bool CancelPending;

  void RunFromFramework(Job *Trigger);
  void CancelChildrenAndSelf();
};

// -----------------------------------------------------------------------------
// IOJob — convenience base for jobs that wait on external IO with a timeout.
//
// Call `ArmTimeout(ms)` immediately before returning `Step::kWaiting`. When
// the timeout fires, the framework sets `ErrCode = kErrTimeout` and re-enters
// `OnStep(this)`. The job is responsible for any IO-side cleanup in either
// `OnStep` (timeout branch) or `OnCancel` (parent finished early).
// -----------------------------------------------------------------------------

class IOJob : public Job {
 public:
  IOJob() : Job(), JobId(0) {}

  // Schedule a timeout; safe to call once per pending IO. The previous timer
  // (if any) is implicitly invalidated by bumping the internal sequence.
  void ArmTimeout(int Millisecond);

  uint32_t GetJobId() const { return JobId; }
  void SetJobId(uint32_t Id) { JobId = Id; }

 protected:
  uint32_t JobId;
};

using FN_CreateJob = std::function<Job *()>;

// -----------------------------------------------------------------------------
// Internal driver — exposed to framework code (worker thread, ITC, time queue)
// only. Business code MUST NOT touch this directly.
// -----------------------------------------------------------------------------

namespace job_impl {

class Driver {
 public:
  // Drive a job from an external event. The job may delete itself
  // synchronously inside this call. Re-entrant calls (e.g. `InvokeChild`
  // inside an `OnStep`) are flattened into a per-thread pending queue so the
  // call stack never deepens with the job graph.
  static void Run(Job *J, Job *Trigger);

  // Cancel a job (and all its descendants) without delivering any further
  // OnStep. The job will be deleted before this call returns.
  static void Cancel(Job *J);

  // Internal: append a (Job, Trigger) pair to the per-thread pending queue.
  // Called by `Job::InvokeChild` and by the kDone -> "notify parent" path.
  // Business code MUST NOT call this directly.
  static void Schedule(Job *J, Job *Trigger);

 private:
  // Drive a single job until it yields (kWaiting / kDone / cancelled).
  // Any spawned children or "notify parent" continuations are appended to
  // the per-thread pending queue rather than recursed into.
  // `DeleteAfterStep`, if non-null, is freed by the framework right after
  // the first OnStep call — used to deliver a finished child to its parent.
  static void DriveOne(Job *J, Job *Trigger, Job *DeleteAfterStep);

  // Remove every pending-queue entry that mentions `Victim` (as J, Trigger,
  // or DeleteAfterStep). Called on out-of-band deletion so that the outer
  // Run loop never reaches a stale pointer. Exposed to `Job` via friendship.
  static void PurgePending(Job *Victim);

  friend class ::wcbot::Job;
};

}  // namespace job_impl

}  // namespace wcbot