#include "deepseekcompletion.h"
#include "servercodetranslation.h"
#include "llm.h"

#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logLLM)

DeepSeekCompletion::DeepSeekCompletion(const QString &url, const AccountProxy &account)
    : BaseNetWork(account)
    , rootUrl(url)
{
    setTimeOut(35000);
}

QPair<int, QString> DeepSeekCompletion::create(const QString &model, DeepSeekConversation &conversation, const QVariantHash &params)
{
    qCDebug(logLLM) << "Creating completion with model:" << model << rootUrl;

    QJsonObject dataObject;
    QJsonArray conversions = conversation.getConversions();
    if (!conversation.getFunctions().isEmpty()) {
        qCInfo(logLLM) << "Adding" << conversation.getFunctions().size() << "functions to request";
        dataObject.insert("tools", transformFunctionList(conversation.getFunctions()));
    }
    dataObject.insert("messages", conversions);
    if (params.contains("temperature"))
        dataObject.insert("temperature", qBound(0.1, params.value("temperature").toDouble(), 1.0));
    dataObject.insert("stream", true);
    dataObject.insert("model", model);
    if (params.contains("max_tokens"))
        dataObject.insert("max_tokens", params.value("max_tokens").toInt());
    if (params.contains(PREDICT_PARAM_THINKINGMODE)) {
        QJsonObject thinking;
        thinking["type"] = params.value(PREDICT_PARAM_THINKINGMODE).toString();
        dataObject.insert("thinking", thinking);
    }

    auto baseresult = request(QUrl(rootUrl), dataObject, nullptr);

    if (baseresult.error != AIServer::NoError) {
        qCWarning(logLLM) << "Request failed with error:" << baseresult.error << baseresult.errorString;
        baseresult.data = ServerCodeTranslation::serverCodeTranslation(baseresult.error, baseresult.errorString).toUtf8();
        return qMakePair(baseresult.error, baseresult.data);
    }

    conversation.update(baseresult.data);
    return qMakePair(0, QString());
}

QJsonArray DeepSeekCompletion::transformFunctionList(const QJsonArray &inputArray) {
    QJsonArray outputArray;

    for (const QJsonValue &value : inputArray) {
        if (!value.isObject()) continue;
        QJsonObject obj = value.toObject();

        QJsonObject wrapperObject;
        wrapperObject["type"] = "function";
        wrapperObject["function"] = obj;

        outputArray.append(wrapperObject);
    }

    return outputArray;
}
