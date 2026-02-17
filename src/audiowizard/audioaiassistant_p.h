#ifndef AIASSISTANT_P_H
#define AIASSISTANT_P_H

#include "uosai_global.h"
#include "audioaiassistant.h"
#include "compliance/audioaiassistantsetting.h"

#include "gui/ttswidget.h"
#include "gui/iatwidget.h"
#include "wrapper/asrwrapper.h"

UOSAI_BEGIN_NAMESPACE

class AudioAiassistantPrivate
{
public:
    explicit AudioAiassistantPrivate(AudioAiassistant *parent);
    ~AudioAiassistantPrivate();
public:
    AudioAiassistant *q = nullptr;
    unsigned long m_lastExecutionTime = 0;
    TtsWidget *m_tts = nullptr;
    IatWidget *m_iat = nullptr;
    AsrWrapper *m_asr = nullptr;
};

UOSAI_END_NAMESPACE

#endif // AIASSISTANT_P_H
