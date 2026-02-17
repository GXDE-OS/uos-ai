#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "instructionworker.h"

#include <QObject>

class EAiPrompt;
class EConversationPrompt;
namespace uos_ai {
class SystemControlInst : public InstructionWorker
{
    Q_OBJECT

public:
    SystemControlInst();

    virtual QString run(const QString &aiReply) override;

    virtual QSharedPointer<EAiPrompt> genPrompt(const QString &userData) override;
};

class LaunchAppInst : public InstructionWorker
{
    Q_OBJECT

public:
    LaunchAppInst();

    virtual QString run(const QString &aiReply) override;

    virtual QSharedPointer<EAiPrompt> genPrompt(const QString &userData) override;
};

class SendMailInst : public InstructionWorker
{
    Q_OBJECT

public:
    SendMailInst();

    virtual QString run(const QString &aiReply) override;

    virtual QSharedPointer<EAiPrompt> genPrompt(const QString &userData) override;

private:
    QJsonObject parserMailContent(const QString &mailContent);
};

class CreateScheduleInst : public InstructionWorker
{
    Q_OBJECT

public:
    CreateScheduleInst();

    virtual QString run(const QString &aiReply) override;

    virtual QSharedPointer<EAiPrompt> genPrompt(const QString &userData) override;

private:
    QJsonObject parserScheduleContent(const QString &scheduleContent);
};

class GenerateImageInst : public InstructionWorker
{
    Q_OBJECT

public:
    GenerateImageInst();

    virtual bool isAvailable(AssistantType assis, LLMChatModel model) const override;

    virtual QString run(const QString &aiReply) override;

    virtual QSharedPointer<EAiPrompt> genPrompt(const QString &userData) override;
};

class SearchOnlineInst : public InstructionWorker
{
    Q_OBJECT

public:
    SearchOnlineInst();

    virtual QString run(const QString &aiReply) override;

    virtual QSharedPointer<EAiPrompt> genPrompt(const QString &userData) override;

private:
    QString search(const QString &text);
};

class MultimediaControlInst : public InstructionWorker
{
    Q_OBJECT

public:
    MultimediaControlInst();

    virtual QString run(const QString &aiReply) override;

    virtual QSharedPointer<EAiPrompt> genPrompt(const QString &userData) override;
};
}
#endif // INSTRUCTIONS_H
