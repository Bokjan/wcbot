#include "DelayQueue.h"

#include <list>
#include <map>

#include "Job.h"

namespace wcbot {

struct JobTimeoutInfo {
  using SteadyTimePoint = std::chrono::time_point<std::chrono::steady_clock>;
  Job *JobPtr;
  SteadyTimePoint TimeoutAt;
  JobTimeoutInfo(Job *J, SteadyTimePoint T) : JobPtr(J), TimeoutAt(T) {}
  bool operator<(const JobTimeoutInfo &Other) const {
    if (TimeoutAt == Other.TimeoutAt) {
      return JobPtr < Other.JobPtr;
    }
    return TimeoutAt < Other.TimeoutAt;
  }
};

class DelayQueueImpl {
  friend class DelayQueue;
  std::list<JobTimeoutInfo> List;
  std::map<Job *, std::list<JobTimeoutInfo>::iterator> Map;
  std::list<JobTimeoutInfo>::iterator FindPosition(JobTimeoutInfo::SteadyTimePoint Position) {
    if (List.empty()) {
      return List.end();
    }
    for (auto RevIt = List.rbegin(); RevIt != List.rend(); ++RevIt) {
      if (RevIt->TimeoutAt < Position) {
        return RevIt.base();
      }
    }
    return List.begin();
  }
};

DelayQueue::DelayQueue() : PImpl(new DelayQueueImpl) {}

DelayQueue::~DelayQueue() { delete PImpl; }

bool DelayQueue::Join(Job *J, int MS) {
  auto MapIt = PImpl->Map.find(J);
  if (MapIt != PImpl->Map.end()) {
    return false;
  }
  std::chrono::milliseconds Period(MS);
  auto Point = std::chrono::steady_clock::now() + Period;
  auto ListIt = PImpl->List.insert(PImpl->FindPosition(Point), JobTimeoutInfo(J, Point));
  PImpl->Map.insert(std::make_pair(J, ListIt));
  return true;
}

Job *DelayQueue::Dequeue(std::chrono::time_point<std::chrono::steady_clock> Now) {
  if (PImpl->List.front().TimeoutAt > Now) {
    return nullptr;
  }
  Job *Ret = PImpl->List.front().JobPtr;
  PImpl->List.erase(PImpl->List.begin());
  auto MapIt = PImpl->Map.find(Ret);
  if (MapIt != PImpl->Map.end()) {
    PImpl->Map.erase(MapIt);
  }
  return Ret;
}

bool DelayQueue::Remove(Job *J) {
  auto MapIt = PImpl->Map.find(J);
  if (MapIt == PImpl->Map.end()) {
    return false;
  }
  PImpl->List.erase(MapIt->second);
  PImpl->Map.erase(MapIt);
  return true;
}

}  // namespace wcbot
