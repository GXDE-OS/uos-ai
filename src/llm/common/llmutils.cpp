#include "llmutils.h"

#include "chatgpt.h"
#include "sparkdesk.h"
#include "360.h"
#include "zhipu.h"
#include "wxqf.h"
#include "localtext2image.h"

#include <QRegExp>
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
    case LLMChatModel::LOCAL_TEXT2IMAGE:
        copilot.reset(new LocalText2Image(serverproxy));
        break;
    }

    return copilot;
}

QString LLMUtils::adjustDbusPath(QString appId)
{
    return DBUS_SERVER_PATH + QString("/") + "a_" + appId.replace(QRegExp("[^A-Za-z]"), "_");
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
