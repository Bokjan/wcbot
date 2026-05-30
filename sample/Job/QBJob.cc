#include "QBJob.h"

#include "wcbot/Job/SilentPushJob.h"
#include "wcbot/Utility/Logger.h"
#include "wcbot/WeCom/TextServerMessage.h"

QBJob::Step QBJob::OnStep(wcbot::Job* /*Trigger*/) {
  switch (State) {
    case StateEnum::kInit: {
      // Cron-fired one-shot: send the daily reminder via a silent push.
      LOG_INFO("%s", "QBJob, 提醒发Q币");
      wcbot::wecom::TextServerMessage TSM;
      TSM.Content = "各位薅薅公子，明天发 Q 币！你的 30 Q 币用完了吗？";
      InvokeChild(new wcbot::SilentPushJob(TSM));
      State = StateEnum::kWaitPush;
      return Step::kWaiting;
    }
    case StateEnum::kWaitPush:
      // SilentPushJob has finished (success or failure); we're done.
      return Step::kDone;
  }
  return Step::kDone;
}