#include "aiassistantttsproxy.h"
#include "wrapper/compliance/aiassistantsetting.h"
#include "wrapper/compliance/aiassistantsubstitute.h"

UOSAI_USE_NAMESPACE

AiassistantTtsProxy::AiassistantTtsProxy(AiassistantSubstitute *parent)
    : QObject(parent)
    , q(parent)
{

}

AiassistantTtsProxy::~AiassistantTtsProxy()
{

}

void AiassistantTtsProxy::setTTSEnable(bool enable)
{
    AiassistantSetting::instance()->setTTSEnable(enable);
}

bool AiassistantTtsProxy::getTTSEnable()
{
    return AiassistantSetting::instance()->getTTSEnable();
}

void AiassistantTtsProxy::setEnableWindow(bool enable)
{
    AiassistantSetting::instance()->setEnableWindow(enable);
}

bool AiassistantTtsProxy::getEnableWindow()
{
    return AiassistantSetting::instance()->getEnableWindow();
}

bool AiassistantTtsProxy::isTTSInWorking()
{
    return q->isTTSInWorking();
}

void AiassistantTtsProxy::stopTTSDirectly()
{
    return q->stopTTSDirectly();
}
