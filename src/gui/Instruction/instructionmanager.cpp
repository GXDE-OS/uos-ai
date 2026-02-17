#include "instructionmanager.h"
#include "uosai_global.h"
#include "instructions.h"
#include "../chat/private/eaicallbck.h"
#include <report/functioncallpoint.h>
#include <report/eventlogutil.h>

#include <QJsonArray>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

InstructionManager *InstructionManager::instance()
{
    static InstructionManager ins;
    return &ins;
}

InstructionManager::InstructionManager()
{
    init();
}

void InstructionManager::init()
{
    QSharedPointer<InstructionWorker> sysCtrInst;
    sysCtrInst.reset(new SystemControlInst());
    QSharedPointer<InstructionWorker> launchAppInst;
    launchAppInst.reset(new LaunchAppInst());
    QSharedPointer<InstructionWorker> sendMailInst;
    sendMailInst.reset(new SendMailInst());
    QSharedPointer<InstructionWorker> createScheduleInst;
    createScheduleInst.reset(new CreateScheduleInst());
    QSharedPointer<InstructionWorker> genImageInst;
    genImageInst.reset(new GenerateImageInst());
    QSharedPointer<InstructionWorker> searchInst;
    searchInst.reset(new SearchOnlineInst());
    QSharedPointer<InstructionWorker> multimediaInst;
    multimediaInst.reset(new MultimediaControlInst());

    instWorkers = {
        {SystemControl, sysCtrInst},
        {LaunchApp, launchAppInst},
        {SendMail, sendMailInst},
        {CreateSchedule, createScheduleInst},
        {GenerateImage, genImageInst},
        {SearchOnline, searchInst},
        {MultimediaControl, multimediaInst},
    };
}

QString InstructionManager::query(AssistantType type, LLMChatModel model)
{
    qCDebug(logAIGUI) << "Querying available instructions for assistant type:" << type << "model:" << model;
    QJsonArray resultArray;
    for (int inst: instWorkers.keys()) {
        QSharedPointer<InstructionWorker> instWorker = instWorkers.value(inst);
        if (!instWorker->isAvailable(type, model))
            continue;

        QJsonObject instObj;
        instObj.insert("tagType", inst);
        instObj.insert("tagName", instWorker->getTagName());
        instObj.insert("content", instWorker->getContent());
        instObj.insert("description", instWorker->getDescription());

        resultArray.append(instObj);
    }

    qCDebug(logAIGUI) << "Found" << resultArray.size() << "available instructions";
    return QJsonDocument(resultArray).toJson(QJsonDocument::Compact);
}

QSharedPointer<EAiPrompt> InstructionManager::genPrompt(int inst, const QString &userData)
{
    QSharedPointer<InstructionWorker> instWorker = instWorkers.value(inst);

    QSharedPointer<EAiPrompt> aiPrompt;
    aiPrompt = instWorker->genPrompt(userData);

    // tid:1001600007 event:functioncall
    ReportIns()->writeEvent(report::FunctioncallPoint(instWorker->getTagNameEn()).assemblingData());

    return aiPrompt;
}

QString InstructionManager::onAiCallback(int inst, QString aiReply)
{
    qCDebug(logAIGUI) << "Processing AI callback for instruction:" << inst;
    QSharedPointer<InstructionWorker> instWorker = instWorkers.value(inst);

    QJsonObject message {
        {"message", QJsonObject {
            {"content", instWorker->run(aiReply)},
            {"chatType", ChatAction::ChatTextPlain}
        }},
        {"stream", false}
    };

    return QJsonDocument(message).toJson();
}
