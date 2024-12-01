#include "aiassistantmainwindowproxy.h"
#include "wrapper/compliance/aiassistantsetting.h"
#include "wrapper/compliance/aiassistantsubstitute.h"

UOSAI_USE_NAMESPACE

AiassistantMainWindowProxy::AiassistantMainWindowProxy(AiassistantSubstitute *parent)
    : QObject(parent)
    , q(parent)
{

}

AiassistantMainWindowProxy::~AiassistantMainWindowProxy()
{

}

void AiassistantMainWindowProxy::SpeechToText()
{

}

void AiassistantMainWindowProxy::TextToSpeech()
{
    q->textToSpeech();
}

void AiassistantMainWindowProxy::TextToTranslate()
{

}



