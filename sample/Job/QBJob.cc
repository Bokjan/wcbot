#include "QBJob.h"

#include "wcbot/Job/SilentPushJob.h"
#include "wcbot/Utility/Logger.h"
#include "wcbot/WeCom/TextServerMessage.h"

void QBJob::Do(Job* Trigger) {
  LOG_INFO("%s", "QBJob, 提醒发Q币");
  wcbot::wecom::TextServerMessage TSM;
  TSM.Content = "各位薅薅公子，明天发 Q 币！你的 30 Q 币用完了吗？";
  (new wcbot::SilentPushJob(this->Worker, TSM))->Do();
  DeleteThis();
}

void QBJob::OnTimeout(Job* Trigger) {
  LOG_ERROR("QBJob timeout");
  DeleteThis();
}
