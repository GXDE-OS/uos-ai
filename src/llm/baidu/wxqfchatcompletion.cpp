#include "wxqfchatcompletion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

WXQFChatCompletion::WXQFChatCompletion(const AccountProxy &account)
    : WXQFNetWork(account)
{

}

QPair<int, QString> WXQFChatCompletion::create(int model, const QString &url, WXQFConversation &conversation, const QVariantHash &params)
{
    QJsonObject dataObject;
    QJsonArray conversions = conversation.getConversions();
    if (!conversation.getFunctions().isEmpty()) {
        dataObject.insert("functions", conversation.getFunctions());
        qCDebug(logLLM) << "WXQF Added functions to request";
    }

    dataObject.insert("messages", conversions);
    if (params.contains("temperature"))
        dataObject.insert("temperature", qBound(0.0, params.value("temperature").toDouble(), 1.0));
    dataObject.insert("stream", true);

    QString modelUrl = url;

    if (modelUrl.isEmpty()) {
        QString path;
        if (model == WXQF_ERNIE_Bot_turbo) {
            path = "/chat/eb-instant";
        }  else if (model == WXQF_ERNIE_Bot_4) {
            path = "/chat/completions_pro";
        } else {
            //path = "/chat/ernie_bot_8k";//ernie_bot_8k下线,迁移到ERNIE-3.5-8K，升级到ernie-3.5-128k
            path = "/chat/ernie-3.5-128k";
        }

        const QString rootPath = "https://aip.baidubce.com/rpc/2.0/ai_custom/v1/wenxinworkshop";

        modelUrl = rootPath + path;
    }

    qCInfo(logLLM) << "WXQF Requesting API with URL:" << modelUrl;
    const QPair<int, QByteArray> &resultPairs = request(dataObject, modelUrl);

    if (resultPairs.first != 0) {
        qCWarning(logLLM) << "WXQF Request failed with error code:" << resultPairs.first;
        return qMakePair(resultPairs.first, resultPairs.second);
    }

    conversation.update(resultPairs.second);
    return qMakePair(0, QString());
}
