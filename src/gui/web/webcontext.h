#ifndef WEBCONTEXT_H
#define WEBCONTEXT_H

#include "appwebview.h"
#include "windowchannel.h"
#include "sessionchannel.h"
#include "assistantchannel.h"
#include "systemchannel.h"
#include "serviceconfigchannel.h"
#include "conversationchannel.h"
#include "filechannel.h"
#include "audiochannel.h"
#include "taskchannel.h"
#include "agent/mcp/skillsmanager.h"
#include "reportchannel.h"

namespace  uos_ai{

struct WebContext
{
    AppWebView *appView = nullptr;
    WindowChannel *winCh = nullptr;
    SessionChannel *sessCh = nullptr;
    AssistantChannel *assistantCh = nullptr;
    SystemChannel *systemCh = nullptr;
    ServiceConfigChannel *serviceConfigCh = nullptr;
    ConversationChannel *conversationCh = nullptr;
    FileChannel *fileCh = nullptr;
    AudioChannel *audioCh = nullptr;
    TaskChannel *taskCh = nullptr;
    SkillsManager *skillsMgr = nullptr;
    ReportChannel *reportCh = nullptr;

    void reset();
};


}

#endif // WEBCONTEXT_H
