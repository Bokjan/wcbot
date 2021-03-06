#include "ITC.h"

#include <deque>
#include <mutex>

#include "EngineImpl.h"
#include "WorkerThread.h"
#include "../Utility/TcpMemoryBuffer.h"

namespace wcbot {

class ItcQueueImpl {
 public:
  std::deque<ItcEvent*> Queue;
  std::mutex Mutex;
};

ItcQueue::ItcQueue() : PImpl(new ItcQueueImpl()) {}

ItcQueue::~ItcQueue() { delete PImpl; }

void ItcQueue::Enqueue(ItcEvent* Event) {
  std::lock_guard<std::mutex> Lock(PImpl->Mutex);
  PImpl->Queue.push_back(Event);
}

bool ItcQueue::TryEnqueue(ItcEvent* Event) {
  if (!PImpl->Mutex.try_lock()) {
    return false;
  }
  PImpl->Queue.push_back(Event);
  PImpl->Mutex.unlock();
  return true;
}

ItcEvent* ItcQueue::Dequeue() {
  std::lock_guard<std::mutex> Lock(PImpl->Mutex);
  if (PImpl->Queue.empty()) {
    return nullptr;
  }
  ItcEvent* Ret = PImpl->Queue.front();
  PImpl->Queue.pop_front();
  return Ret;
}

ItcEvent* ItcQueue::TryDequeue() {
  if (!PImpl->Mutex.try_lock()) {
    return nullptr;
  }
  if (PImpl->Queue.empty()) {
    return nullptr;
  }
  ItcEvent* Ret = PImpl->Queue.front();
  PImpl->Queue.pop_front();
  return Ret;
}

bool ItcQueue::IsEmpty() {
  std::lock_guard<std::mutex> Lock(PImpl->Mutex);
  return PImpl->Queue.empty();
}

void ItcQueue::Clear() {
  std::lock_guard<std::mutex> Lock(PImpl->Mutex);
  PImpl->Queue.clear();
}

namespace itc {

void TcpMainToWorker::Process() {
  // dispatch TCP request
  worker_impl::DispatchTcp(this->Buffer);
  // free memory
  this->DeleteThis();
}

void TcpWorkerToMain::Process() {
  // do uv write
  main_impl::SendTcpToClient(Buffer, ConnId, CloseConnection);
  // free memory
  this->DeleteThis();
}

void JobCreateAndRun::Process() {
  Function()->Do();
  this->DeleteThis();
}

}  // namespace itc

}  // namespace wcbot
