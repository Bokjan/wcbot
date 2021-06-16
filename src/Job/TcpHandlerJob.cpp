#include "TcpHandlerJob.h"
#include "Core/ITC.h"
#include "Core/WorkerThread.h"

namespace wcbot {

void TcpHandlerJob::SendData(MemoryBufferPtr Buffer, bool CloseConnection) {
  itc::TcpWorkerToMain *Event =
      new itc::TcpWorkerToMain(this->Worker->EImpl, Buffer, this->ReceiveBuffer->ClientTcpId);
  if (CloseConnection) {
    Event->SetCloseConnection();
  }
  this->Worker->WorkerToMainQueue.Enqueue(Event);
  this->Worker->NotifyMain();
}

}  // namespace wcbot
