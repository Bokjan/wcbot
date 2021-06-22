#include "EchoCallbackJob.h"

#include "wcbot/WeCom/TextClientMessage.h"
#include "wcbot/WeCom/TextServerMessage.h"

void EchoCallbackJob::Do(Job *Trigger) {
  Job::Do(Trigger);
  do {
    auto *TCM = dynamic_cast<wcbot::wecom::TextClientMessage *>(Request);
    if (TCM == nullptr) {
      break;
    }
    auto *TSM = new wcbot::wecom::TextServerMessage;
    TSM->Content = TCM->Content;
    this->SetResponse(TSM);
  } while (false);
  DeleteThis();
}
