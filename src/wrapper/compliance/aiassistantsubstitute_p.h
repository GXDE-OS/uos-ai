#ifndef AIASSISTANTSUBSTITUTE_P_H
#define AIASSISTANTSUBSTITUTE_P_H

#include "uosai_global.h"
#include "aiassistantsubstitute.h"
#include "aiassistantsetting.h"

#include "gui/compliance/ttscompositewidget.h"

UOSAI_BEGIN_NAMESPACE

class AiassistantSubstitutePrivate
{
public:
    explicit AiassistantSubstitutePrivate(AiassistantSubstitute *parent);
    ~AiassistantSubstitutePrivate();
    QString getSelectedString();
public:
    AiassistantSubstitute *q = nullptr;
    unsigned long m_lastExecutionTime = 0;
    TtsCompositeWidget *m_tts = nullptr;
    QString m_ttsContent;

};


UOSAI_END_NAMESPACE

#endif // AIASSISTANTSUBSTITUTE_P_H
