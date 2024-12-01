#include "aiassistanttransproxy.h"
#include "wrapper/compliance/aiassistantsetting.h"
#include "wrapper/compliance/aiassistantsubstitute.h"

UOSAI_USE_NAMESPACE

AiassistantTransProxy::AiassistantTransProxy(AiassistantSubstitute *parent)
    : QObject(parent)
    , q(parent)
{

}

AiassistantTransProxy::~AiassistantTransProxy()
{

}

void AiassistantTransProxy::setTransEnable(bool on)
{
    AiassistantSetting::instance()->setTransEnable(on);
}

bool AiassistantTransProxy::getTransEnable()
{
    return AiassistantSetting::instance()->getTransEnable();
}

void AiassistantTransProxy::setTransLanguage(const QString &language)
{
    AiassistantSetting::instance()->setTransLanguage(language);
}

QString AiassistantTransProxy::getTransLanguage()
{
    return AiassistantSetting::instance()->getTransLanguage();
}
