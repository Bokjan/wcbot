#include "TimeQueue.h"

#include <list>
#include <map>

#include "../Job/Job.h"

namespace wcbot {

struct JobTimeoutInfo {
  using SteadyTimePoint = std::chrono::time_point<std::chrono::steady_clock>;
  uint32_t Id;
  IOJob *JobPtr;
  SteadyTimePoint TimeoutAt;
  JobTimeoutInfo(uint32_t Id, IOJob *J, SteadyTimePoint T) : Id(Id), JobPtr(J), TimeoutAt(T) {}
};

class DelayQueueImpl {
  friend class DelayQueue;
  uint32_t JobSeq;
  std::list<JobTimeoutInfo> List;
  std::map<uint32_t, std::list<JobTimeoutInfo>::iterator> Map;

  DelayQueueImpl() : JobSeq(0) {}

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

void DelayQueue::Join(IOJob *J, int MS) {
  J->SetJobId(PImpl->JobSeq++);
  std::chrono::milliseconds Period(MS);
  auto Point = std::chrono::steady_clock::now() + Period;
  auto ListIt =
      PImpl->List.insert(PImpl->FindPosition(Point), JobTimeoutInfo(J->GetJobId(), J, Point));
  PImpl->Map.insert(std::make_pair(J->GetJobId(), ListIt));
}

IOJob *DelayQueue::Dequeue(std::chrono::time_point<std::chrono::steady_clock> Now) {
  if (PImpl->List.empty()) {
    return nullptr;
  }
  for (;;) {
    if (PImpl->List.front().TimeoutAt > Now) {
      return nullptr;
    }
    JobTimeoutInfo Front = PImpl->List.front();
    PImpl->List.erase(PImpl->List.begin());
    auto MapIt = PImpl->Map.find(Front.Id);
    if (MapIt == PImpl->Map.end()) {
      PImpl->Map.erase(MapIt);
    }
    if (Front.Id != Front.JobPtr->GetJobId()) {
      continue;
    }
    return Front.JobPtr;
  }
}

IOJob *DelayQueue::Remove(uint32_t Id) {
  auto MapIt = PImpl->Map.find(Id);
  if (MapIt == PImpl->Map.end()) {
    return nullptr;
  }
  IOJob *Ret = MapIt->second->JobPtr;
  PImpl->List.erase(MapIt->second);
  PImpl->Map.erase(MapIt);
  return Ret;
}

struct JobSleepInfo {
  using SteadyTimePoint = std::chrono::time_point<std::chrono::steady_clock>;
  Job *JobPtr;
  SteadyTimePoint WakeUpAt;
  JobSleepInfo(Job *J, SteadyTimePoint T) : JobPtr(J), WakeUpAt(T) {}
};

class SleepQueueImpl {
  friend class SleepQueue;
  std::list<JobSleepInfo> List;

  SleepQueueImpl() {}

  std::list<JobSleepInfo>::iterator FindPosition(JobSleepInfo::SteadyTimePoint Position) {
    if (List.empty()) {
      return List.end();
    }
    for (auto RevIt = List.rbegin(); RevIt != List.rend(); ++RevIt) {
      if (RevIt->WakeUpAt < Position) {
        return RevIt.base();
      }
    }
    return List.begin();
  }
};

SleepQueue::SleepQueue() : PImpl(new SleepQueueImpl()) {}

SleepQueue::~SleepQueue() { delete PImpl; }

void SleepQueue::Join(Job *J, int Millisecond) {
  std::chrono::milliseconds Period(Millisecond);
  auto Point = std::chrono::steady_clock::now() + Period;
  PImpl->List.insert(PImpl->FindPosition(Point), JobSleepInfo(J, Point));
}

Job *SleepQueue::Dequeue(std::chrono::time_point<std::chrono::steady_clock> Now) {
  if (PImpl->List.empty()) {
    return nullptr;
  }
  auto &Front = PImpl->List.front();
  if (Front.WakeUpAt > Now) {
    return nullptr;
  }
  auto Ret = Front.JobPtr;
  PImpl->List.erase(PImpl->List.begin());
  return Ret;
}

}  // namespace wcbot
