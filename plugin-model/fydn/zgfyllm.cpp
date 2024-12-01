// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "zgfyllm.h"
#include "fydnnetwork.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

using namespace uos_ai;
using namespace uos_ai::fydn;

ZgfyLLM::ZgfyLLM() : QObject(), LLMModel()
{

}

QString ZgfyLLM::model() const
{
    return modelID();
}

QJsonObject ZgfyLLM::generate(const QString &content, const QVariantHash &params, LLMModel::streamFuncion stream, void *user)
{
    float temperature = params.value(GENERATE_PARAM_TEMPERATURE, 0.9).toFloat();
    QString role = params.value(GENERATE_PARAM_ROLE).toString();

    QJsonObject sendObj;
    QJsonArray conversions = QJsonDocument::fromJson(content.toUtf8()).array();
    sendObj.insert("model", model());
    sendObj.insert("messages", conversions);
    sendObj.insert("stream", true);

    QString url = "https://api.cjbdi.com:8443/354347/v1/chat/completions";
    if (role == roleFlfg()) {
        url = "https://api.cjbdi.com:8443/354347/llm/flfg/v1/chat/completions";
        sendObj.insert("top_k", 5);
        sendObj.insert("threshold", 0.5);
    }

    FydnNetwork *fydnNetwork = new FydnNetwork();
    QString responseContent;
    connect(fydnNetwork, &FydnNetwork::sigReadStream, [=, &responseContent](const QString &data){
        if (!stream(data, user)) {
            responseContent = "task cancel";
            fydnNetwork->setAbortRequest();
        }
        else
            responseContent += data;
    });

    QPair<int, QString> errorPair = fydnNetwork->request(sendObj, url);

    if (errorPair.first == FydnNetwork::ErrorType::NetWorkError)
        errorPair.second = QCoreApplication::translate("ErrorCodeTranslation", "Connection exception, please try again later.");

    delete fydnNetwork;
    fydnNetwork = nullptr;

    QJsonObject response;
    response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
    response.insert(GENERATE_RESPONSE_CODE, errorPair.first);
    response.insert(GENERATE_RESPONSE_ERRORMSG, errorPair.second);

    return response;
}
