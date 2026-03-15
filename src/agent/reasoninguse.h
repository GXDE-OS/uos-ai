#ifndef REASONINGUSE_H
#define REASONINGUSE_H

#include <QObject>

namespace uos_ai {
class LlmAgent;
struct ReasoningUse
{
    enum TitleStatus {
        InProgress,
        Completed,
        Failed
    };

    static void reasoningUseContent(LlmAgent *agent, const QString &content);
    static void reasoningUseTitle(LlmAgent *agent, const QString &content, TitleStatus status = InProgress);
};
}
#endif // REASONINGUSE_H
