#include "webcontext.h"

using namespace uos_ai;

void WebContext::reset()
{
    if (appView)
        delete appView;
    appView = nullptr;

    delete winCh;
    winCh = nullptr;

    delete sessCh;
    sessCh = nullptr;

    delete assistantCh;
    assistantCh = nullptr;

    delete systemCh;
    systemCh = nullptr;

    delete serviceConfigCh;
    serviceConfigCh = nullptr;

    delete conversationCh;
    conversationCh = nullptr;

    delete fileCh;
    fileCh = nullptr;

    delete audioCh;
    audioCh = nullptr;

    delete taskCh;
    taskCh = nullptr;

    delete skillsMgr;
    skillsMgr = nullptr;
}
