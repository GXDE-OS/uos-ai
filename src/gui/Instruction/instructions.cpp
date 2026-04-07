#include "instructions.h"
#include "uosai_global.h"
#include "functionhandler.h"
#include "instructionmanager.h"
#include "../chat/private/eappaiprompt.h"
#include "util.h"

#include <QJsonArray>
#include <QRegularExpression>
#include <QUrl>
#include <QLoggingCategory>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

SystemControlInst::SystemControlInst()
{
    availableAssis = {AssistantType::UOS_AI};
    availableManu = {ModelManufacturer::BAIDU_WXQF};
    availableLlm = {LLMChatModel::LOCAL_YOURONG_1_5B, LLMChatModel::LOCAL_YOURONG_7B, UOS_FREE};

    tagName = tr("System Control");
    tagNameEn = "System Control";
    defaultContent = tr("Switch to a new wallpaper. set the screen brightness to 30%...");
    description = QString("对系统一些配置的控制，例如网络、蓝牙、壁纸");
}

QString SystemControlInst::run(const QString &aiReply)
{
    // systemControl是单纯的FunctionCall，目前按照FC执行结果直接返回
    return aiReply;
}

QSharedPointer<EAiPrompt> SystemControlInst::genPrompt(const QString &userData)
{
    QSharedPointer<EAiPrompt> aiPrompt;
    aiPrompt.reset(new EAiInstructionPrompt(userData));
    aiPrompt->setReqType(EAiPrompt::RequstType::FunctionCall);
    aiPrompt->setInstType(InstructionManager::Instructions::SystemControl);
    aiPrompt->setFunctions(FunctionHandler::queryInstFunctions(QString("SystemControl")));

    return aiPrompt;
}

LaunchAppInst::LaunchAppInst()
{
    availableAssis = {AssistantType::UOS_AI};
    availableManu = {ModelManufacturer::BAIDU_WXQF};
    availableLlm = {LLMChatModel::LOCAL_YOURONG_1_5B, LLMChatModel::LOCAL_YOURONG_7B, UOS_FREE};

    tagName = tr("Launch or Close App");
    tagNameEn = "Launch or Close App";
    defaultContent = tr("WPS, Music, Album, Control Center, Log Viewer ...");
    description = QString("打开系统中的应用程序");
}

QString LaunchAppInst::run(const QString &aiReply)
{
    // 按照FC实现
    return aiReply;
}

QSharedPointer<EAiPrompt> LaunchAppInst::genPrompt(const QString &userData)
{
    QSharedPointer<EAiPrompt> aiPrompt;
    aiPrompt.reset(new EAiInstructionPrompt(userData));
    aiPrompt->setReqType(EAiPrompt::RequstType::FunctionCall);
    aiPrompt->setInstType(InstructionManager::Instructions::LaunchApp);
    aiPrompt->setFunctions(FunctionHandler::queryInstFunctions(QString("LaunchApp")));

    return aiPrompt;
}

SendMailInst::SendMailInst()
{
    availableAssis = {AssistantType::UOS_AI};
    availableManu = {ModelManufacturer::BAIDU_WXQF};
    availableLlm = {LLMChatModel::LOCAL_YOURONG_1_5B, LLMChatModel::LOCAL_YOURONG_7B, UOS_FREE};

    tagName = tr("Send Mail");
    tagNameEn = "Send Mail";
    defaultContent = tr("Help me send an email to [Recipient's Name], with the content: [Email Content].");
    description = QString("根据用户描述发送一封邮件");
}

QString SendMailInst::run(const QString &aiReply)
{
    QJsonObject mailObj = parserMailContent(aiReply);
    if (mailObj.isEmpty()) {
        qCWarning(logAIGUI) << "Failed to parse mail content";
        return tr("Sorry, no matching operations were found.");
    }

    QJsonObject mailFunc;
    mailFunc.insert("name", "sendMail");
    mailFunc.insert("arguments", QString::fromUtf8(QJsonDocument(mailObj).toJson()));

    QJsonObject funcResult = FunctionHandler::instFuncProcess(mailFunc, nullptr);

    return funcResult.value("description").toString();
}

QSharedPointer<EAiPrompt> SendMailInst::genPrompt(const QString &userData)
{
    QString promptTemplate = tr("Please extract the relevant parameters from the provided input to match the following JSON email template format," \
                                "and output only the JSON content. Do not include any extraneous information.\n\n" \
                                "Email template format:\njson\n{\"subject\": \"Email Subject\", \"content\": \"Email Body\", \"to\": \"Recipient\", \"cc\": \"CC Recipient\", \"bcc\": \"BCC Recipient\"}\n\n"
                                "Input: %1");
    QString prompt = promptTemplate.arg(userData);

    QSharedPointer<EAiPrompt> aiPrompt;
    aiPrompt.reset(new EAiInstructionPrompt(prompt));
    aiPrompt->setReqType(EAiPrompt::RequstType::TextPlain);
    aiPrompt->setInstType(InstructionManager::Instructions::SendMail);

    return aiPrompt;
}

QJsonObject SendMailInst::parserMailContent(const QString &mailContent)
{
    QRegularExpression regex(R"(\{(?:[^{}]|(?R))*\})");
    QRegularExpressionMatch match = regex.match(mailContent);

    QJsonObject mailObj;
    if (match.hasMatch()) {
        QString jsonString = match.captured(0);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString.toUtf8());

        mailObj = jsonDoc.object();
    } else {
        qCWarning(logAIGUI) << "No valid JSON found in mail content";
    }

    return mailObj;
}

CreateScheduleInst::CreateScheduleInst()
{
    availableAssis = {AssistantType::UOS_AI};
    availableManu = {ModelManufacturer::BAIDU_WXQF};
    availableLlm = {LLMChatModel::LOCAL_YOURONG_1_5B, LLMChatModel::LOCAL_YOURONG_7B, UOS_FREE};

    tagName = tr("Create Schedule");
    tagNameEn = "Create Schedule";
    defaultContent = tr("Schedule a meeting with the Marketing Department from 2 PM to 5 PM.");
    description = QString("创建一个用户日程，并保存在日历中");
}

QString CreateScheduleInst::run(const QString &aiReply)
{
    QJsonObject scheduleObj = parserScheduleContent(aiReply);
    if (scheduleObj.isEmpty()) {
        qCWarning(logAIGUI) << "Failed to parse schedule content";
        return tr("Sorry, no matching operations were found.");
    }

    QJsonObject scheduleFunc;
    scheduleFunc.insert("name", "createSchedule");
    scheduleFunc.insert("arguments", QString::fromUtf8(QJsonDocument(scheduleObj).toJson()));

    QJsonObject funcResult = FunctionHandler::instFuncProcess(scheduleFunc, nullptr);

    return funcResult.value("description").toString();
}

QSharedPointer<EAiPrompt> CreateScheduleInst::genPrompt(const QString &userData)
{
    QString promptTemplate = tr("Please parse the corresponding parameters from the input content to match the schedule template in the following JSON format," \
                                "and output only the JSON content, excluding any irrelevant information.\n\n" \
                                "Schedule template format:\njson\n{\"subject\": \"Subject\", \"startTime\": \"Schedule Start Time, format: yyyy-MM-ddThh:mm:ss\", \"endTime\": \"Schedule End Time, format: yyyy-MM-ddThh:mm:ss\"}\n\n" \
                                "Input: %1\n" \
                                "Current time: %2");
    QString prompt = promptTemplate.arg(userData).arg(QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss"));

    QSharedPointer<EAiPrompt> aiPrompt;
    aiPrompt.reset(new EAiInstructionPrompt(prompt));
    aiPrompt->setReqType(EAiPrompt::RequstType::TextPlain);
    aiPrompt->setInstType(InstructionManager::Instructions::CreateSchedule);

    return aiPrompt;
}

QJsonObject CreateScheduleInst::parserScheduleContent(const QString &scheduleContent)
{
    QRegularExpression regex(R"(\{(?:[^{}]|(?R))*\})");
    QRegularExpressionMatch match = regex.match(scheduleContent);

    QJsonObject scheduleObj;
    if (match.hasMatch()) {
        QString jsonString = match.captured(0);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString.toUtf8());

        scheduleObj = jsonDoc.object();
    } else {
        qCWarning(logAIGUI) << "No valid JSON found in schedule content";
    }

    return scheduleObj;
}

GenerateImageInst::GenerateImageInst()
{
    availableAssis = {AssistantType::UOS_AI};
    availableManu = {ModelManufacturer::BAIDU_WXQF};

    tagName = tr("Generate Image");
    tagNameEn = "Generate Image";
    defaultContent = tr("Describe Image Content");
    description = QString("调用大模型，文生图");
}

bool GenerateImageInst::isAvailable(AssistantType assis, LLMChatModel model) const
{
#ifdef TEXT_TO_IMAGE
    if (assis == AssistantType::UOS_AI)
        return true;
#endif
    return availableAssis.contains(assis)
            && (availableManu.contains(LLMServerProxy::llmManufacturer(model))
                || availableLlm.contains(model));
}

QString GenerateImageInst::run(const QString &aiReply)
{
    Q_UNUSED(aiReply)

    return QString("");
}

QSharedPointer<EAiPrompt> GenerateImageInst::genPrompt(const QString &userData)
{
    QSharedPointer<EAiPrompt> aiPrompt;
    aiPrompt.reset(new EAiInstructionPrompt(userData));
    aiPrompt->setReqType(EAiPrompt::RequstType::Text2Image);
    aiPrompt->setInstType(InstructionManager::Instructions::GenerateImage);

    return aiPrompt;
}

SearchOnlineInst::SearchOnlineInst()
{
    availableAssis = {AssistantType::UOS_AI};
    availableManu = {ModelManufacturer::BAIDU_WXQF, ModelManufacturer::XUNFEI_SPARKDESK, ModelManufacturer::GPT360,
                   ModelManufacturer::ZHIPUGLM, ModelManufacturer::MODELHUB, ModelManufacturer::PRIVATE,
                   ModelManufacturer::GEMINI, ModelManufacturer::CHATGPT, ModelManufacturer::OPENAI_API,
                   ModelManufacturer::MODEL_LOCAL, ModelManufacturer::OPENROUTER_API, ModelManufacturer::DEEPSEEK};

    tagName = tr("Search Online");
    tagNameEn = "Search Online";
    defaultContent = tr("Enter Search Content");
    description = QString("调用默认浏览器，唤起360 AI 搜索，搜索用户输出的内容");
}

QString SearchOnlineInst::run(const QString &aiReply)
{
    return search(aiReply);
}

QSharedPointer<EAiPrompt> SearchOnlineInst::genPrompt(const QString &userData)
{
    QSharedPointer<EAiPrompt> aiPrompt;
    aiPrompt.reset(new EAiInstructionPrompt(userData));
    aiPrompt->setReqType(EAiPrompt::RequstType::Search);
    aiPrompt->setInstType(InstructionManager::Instructions::SearchOnline);

    return aiPrompt;
}

QString SearchOnlineInst::search(const QString &text)
{
    static const int MAX_TEXT_SIZE = 905;
    QString subtext = text;
    if (subtext.size() > MAX_TEXT_SIZE) {
        subtext = subtext.mid(0, MAX_TEXT_SIZE);
    }
    subtext.replace('\n', " ");
    const QString URL = "https://www.sou.com/?q=" + QUrl::toPercentEncoding(subtext) + "&src=360tob_union_add";

    if (!Util::launchUosBrowser(URL)) {
        qCWarning(logAIGUI) << "Failed to launch UOS browser, trying default browser";
        qCInfo(logAIGUI) << "Launching default browser with URL:" << URL;
        Util::launchDefaultBrowser(URL);
    }
    return URL;
}

MultimediaControlInst::MultimediaControlInst()
{
    availableAssis = {AssistantType::UOS_AI};
    availableManu = {ModelManufacturer::BAIDU_WXQF};
    availableLlm = {LLMChatModel::LOCAL_YOURONG_1_5B, LLMChatModel::LOCAL_YOURONG_7B, UOS_FREE};

    tagName = tr("Multimedia Control");
    tagNameEn = "Multimedia Control";

    defaultContent = tr("Play music, state control, seek...");
    description = QString("控制多媒体播放，包括播放、暂停、切换曲目等");
}

QString MultimediaControlInst::run(const QString &aiReply)
{
    // MultimediaControl是FunctionCall，按照FC执行结果直接返回
    qCInfo(logAIGUI) << "Executing multimedia control with AI reply:" << aiReply;
    return aiReply;
}

QSharedPointer<EAiPrompt> MultimediaControlInst::genPrompt(const QString &userData)
{
    QSharedPointer<EAiPrompt> aiPrompt;
    aiPrompt.reset(new EAiInstructionPrompt(userData));
    aiPrompt->setReqType(EAiPrompt::RequstType::FunctionCall);
    aiPrompt->setInstType(InstructionManager::Instructions::MultimediaControl);
    aiPrompt->setFunctions(FunctionHandler::queryInstFunctions(QString("MultimediaControl")));

    return aiPrompt;
}
