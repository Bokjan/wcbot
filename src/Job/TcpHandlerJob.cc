#include "TcpHandlerJob.h"

#include "../Core/ITC.h"
#include "../Core/WorkerThread.h"

namespace wcbot {

TcpHandlerJob::~TcpHandlerJob() { delete ReceiveBuffer; }

void TcpHandlerJob::SendData(MemoryBuffer* Buffer, bool CloseConnection) {
  itc::TcpWorkerToMain* Event =
      new itc::TcpWorkerToMain(Buffer, this->ReceiveBuffer->ClientTcpId);
  if (CloseConnection) {
    Event->SetCloseConnection();
  }
  worker_impl::g_ThisThread->WorkerToMainQueue.Enqueue(Event);
  worker_impl::g_ThisThread->NotifyMain();
}

}  // namespace wcbot
