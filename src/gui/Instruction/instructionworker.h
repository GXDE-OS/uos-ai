#ifndef INSTRUCTIONWORKER_H
#define INSTRUCTIONWORKER_H

#include "serverdefs.h"

#include <QObject>

class EAiPrompt;
class EAiCallback;
namespace uos_ai {
class InstructionWorker : public QObject
{
    Q_OBJECT

public:
    InstructionWorker();

    virtual ~InstructionWorker();

    virtual bool isAvailable(AssistantType assis, LLMChatModel model) const;
    virtual QString getTagName() const;
    virtual QString getTagNameEn() const; // 埋点专用
    virtual QString getContent() const;
    virtual QString getDescription() const;

    virtual QSharedPointer<EAiPrompt> genPrompt(const QString &userData) = 0;
    virtual QString run(const QString &aiReply) = 0;

protected:
    QVector<AssistantType> availableAssis;
    QVector<ModelManufacturer> availableManu;
    QVector<LLMChatModel> availableLlm;

    QString tagName;
    QString tagNameEn; // 埋点专用
    QString defaultContent;
    QString description;
};
}
#endif // INSTRUCTIONWORKER_H
