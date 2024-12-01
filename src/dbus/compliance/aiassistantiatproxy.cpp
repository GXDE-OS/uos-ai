#include "aiassistantiatproxy.h"
#include "wrapper/compliance/aiassistantsetting.h"
#include "wrapper/compliance/aiassistantsubstitute.h"

UOSAI_USE_NAMESPACE

AiassistantIatProxy::AiassistantIatProxy(AiassistantSubstitute *parent)
    : QObject(parent)
    , q(parent)
{

}

AiassistantIatProxy::~AiassistantIatProxy()
{

}

void AiassistantIatProxy::setIatEnable(bool on)
{
    AiassistantSetting::instance()->setIatEnable(on);
}

bool AiassistantIatProxy::getIatEnable()
{
    return AiassistantSetting::instance()->getIatEnable();
}

