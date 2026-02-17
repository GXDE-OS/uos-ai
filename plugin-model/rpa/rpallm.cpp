// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rpallm.h"
#include "rpanetwork.h"
#include "oafunctionhandler.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

using namespace uos_ai;
using namespace uos_ai::rpa;

RpaLLM::RpaLLM() : QObject(), LLMModel()
{
    m_funcs = OaFunctionHandler::queryFunctions();
}

QString RpaLLM::model() const
{
    return modelID();
}

QJsonObject RpaLLM::generate(const QString &content, const QVariantHash &params, LLMModel::streamFuncion stream, void *user)
{
    float temperature = params.value(GENERATE_PARAM_TEMPERATURE, 0.9).toFloat();

    QJsonObject dataObject;
    QJsonArray conversions = QJsonDocument::fromJson(content.toUtf8()).array();
    if (!m_funcs.isEmpty()) {
        dataObject.insert("functions", m_funcs);
    }
    dataObject.insert("messages", conversions);
    dataObject.insert("temperature", temperature);
    dataObject.insert("stream", true);

    RpaNetwork *rpaNetwork = new RpaNetwork();
    QString modelUrl = "https://aip.baidubce.com/rpc/2.0/ai_custom/v1/wenxinworkshop/chat/completions";

    QString responseContent;
    connect(this, &RpaLLM::sigAbort, this, [=, &responseContent](){
        rpaNetwork->setAbortRequest();
        responseContent = "task cancel";
    });
    connect(rpaNetwork, &RpaNetwork::sigReadStream, this, [=, &responseContent](const QString &data){
        stream(data, user);
        responseContent += data;
    });

    QPair<int, QString> resultPairs = rpaNetwork->request(dataObject, modelUrl);

    if (resultPairs.first == RpaNetwork::ErrorType::NetWorkError)
        resultPairs.second = QCoreApplication::translate("ErrorCodeTranslation", "Connection exception, please try again later.");

    delete rpaNetwork;
    rpaNetwork = nullptr;

    QJsonObject response;
    response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
    response.insert(GENERATE_RESPONSE_CODE, resultPairs.first);
    response.insert(GENERATE_RESPONSE_ERRORMSG, resultPairs.second);

    return response;
}

void RpaLLM::setAbort()
{
    emit sigAbort();
}
