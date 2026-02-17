#include "llmutils.h"

#include "chatgpt.h"
#include "sparkdesk.h"
#include "360.h"
#include "zhipu.h"
#include "wxqf.h"
#include "localtext2image.h"
#include "universalapi.h"
#include "openrouterapi.h"
#include "gemini/gemini_1_5.h"
#include "universalagentapi.h"
#include "coze/cozeagent.h"
#include "deepseek/deepseekai.h"
#include "deepseek/deepseekfree.h"

#include <QFile>
#include <QtDBus>
#include <QJsonDocument>

static const QString &appConfigPath = "/appcommands.config";

QSharedPointer<LLM> LLMUtils::getCopilot(const LLMServerProxy &serverproxy)
{
    QSharedPointer<LLM> copilot;

    switch (serverproxy.model) {
    case LLMChatModel::CHATGPT_3_5:
    case LLMChatModel::CHATGPT_3_5_16K:
    case LLMChatModel::CHATGPT_4:
    case LLMChatModel::CHATGPT_4_32K:
        copilot.reset(new ChatGpt(serverproxy));
        break;
    case LLMChatModel::GPT360_S2_V9:
        copilot.reset(new Gpt360(serverproxy));
        break;
    case LLMChatModel::ChatZHIPUGLM_PRO:
    case LLMChatModel::ChatZHIPUGLM_STD:
    case LLMChatModel::ChatZHIPUGLM_LITE:
        copilot.reset(new ZhiPuAI(serverproxy));
        break;
    case LLMChatModel::SPARKDESK:
    case LLMChatModel::SPARKDESK_2:
    case LLMChatModel::SPARKDESK_3:
        copilot.reset(new SparkDesk(serverproxy));
        break;
    case LLMChatModel::WXQF_ERNIE_Bot:
    case LLMChatModel::WXQF_ERNIE_Bot_turbo:
    case LLMChatModel::WXQF_ERNIE_Bot_4:
        copilot.reset(new WXQFAI(serverproxy));
        break;
    case LLMChatModel::GEMINI_1_5_FLASH:
    case LLMChatModel::GEMINI_1_5_PRO:
        copilot.reset(new uos_ai::Gemini_1_5(serverproxy));
        break;
    case LLMChatModel::COZE_AGENT:
        copilot.reset(new uos_ai::CozeAgent(serverproxy));
        break;
    case LLMChatModel::LOCAL_TEXT2IMAGE:
        copilot.reset(new LocalText2Image(serverproxy));
        break;
    case LLMChatModel::OPENAI_API_COMPATIBLE:
    case LLMChatModel::PRIVATE_MODEL:
        copilot.reset(new uos_ai::UniversalAPI(serverproxy));
        break;
    case LLMChatModel::OPENAI_API_WITH_AGENT:
        copilot.reset(new uos_ai::UniversalAgentAPI(serverproxy));
        break;
    case LLMChatModel::OPENROUTER_MODEL:
        copilot.reset(new uos_ai::OpenRouterAPI(serverproxy));
        break;
    case LLMChatModel::DeepSeek_R1:
        copilot.reset(new uos_ai::DeepSeekAI(serverproxy));
        break;
    case::LLMChatModel::DeepSeek_Uos_Free:
        copilot.reset(new uos_ai::DeepSeekFree(serverproxy));
        break;
    }

    return copilot;
}

QString LLMUtils::adjustDbusPath(QString appId)
{
    return DBUS_SERVER_PATH + QString("/") + "a_" + appId.replace(QRegularExpression("[^A-Za-z]"), "_");
}

QString LLMUtils::queryAppId(const uint &pid)
{
    QString processName;
    QString procFilePath = QString("/proc/%1/cmdline").arg(pid);

    QFile file(procFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QStringList args = QString(data).split('/');

        if (!args.isEmpty()) {
            processName = args.last();
        } else {
            qWarning() << "Error: cmdline" << procFilePath;
        }
    } else {
        qWarning() << "Error: Unable to open" << procFilePath;
    }

    return processName;
}

QString LLMUtils::systemEnvInfo()
{
    return QCoreApplication::translate("LLMUtils", "Your system environment is Linux, and the user home path is") + QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + ";";
}
