// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ztbllm.h"
#include "llm/common/networkdefs.h"
#include "wrapper/serverdefs.h"

#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>

using namespace uos_ai;
using namespace ztb;

ZtbLLM::ZtbLLM(QObject *parent)
    : QObject(parent)
    , LLMModel()
{

}

QString ZtbLLM::model() const
{
    return modelID();
}

QJsonObject ZtbLLM::generate(const QString &content, const QVariantHash &params, LLMModel::streamFuncion stream, void *user)
{
    QJsonObject sendObj;
    QJsonArray conversions = QJsonDocument::fromJson(content.toUtf8()).array();
    QString question;
    QJsonArray history;
    if (!conversions.isEmpty()) {
        QJsonObject obj =  conversions.takeAt(conversions.size() - 1).toObject();
        question = obj.value("content").toString();

        QJsonArray qa;
        for (const QJsonValue &tmp : conversions) {
            qa.append(tmp.toObject().value("content").toString());
            if (qa.size() == 2) {
                history.append(qa);
                qa = QJsonArray();
            }
        }
    }

    sendObj.insert("question", question);
    if (!history.isEmpty())
        sendObj.insert("chat_history", history);
    //sendObj.insert("filters", "");

    QJsonDocument jsonDocument(sendObj);
    QByteArray sendData = jsonDocument.toJson(QJsonDocument::Compact);

    QUrl url("http://10.10.16.11:8502/ztb/chat");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkAccessManager manager;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(5 * 60 * 1000);

    QByteArray replyData;
    QNetworkReply *reply = manager.post(request, sendData);

    QObject::connect(reply, &QNetworkReply::readyRead, this, [=, &timer, &replyData](){
        timer.start();
        replyData += reply->readAll();
    });

    timer.start();

    AIServer::ErrorType serverErrorCode = AIServer::ErrorType::NoError;
    QString errorStr;
    QObject::connect(&timer, &QTimer::timeout, this, [=, &loop, &serverErrorCode](){
        serverErrorCode = AIServer::ErrorType::TimeoutError;
        reply->abort();
        loop.quit();
    });

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(this, &ZtbLLM::sigAbort, reply, &QNetworkReply::abort);
    QObject::connect(this, &ZtbLLM::sigAbort, &loop, &QEventLoop::quit);

    loop.exec();
    timer.stop();

    if (serverErrorCode == AIServer::ErrorType::NoError && reply->error() != QNetworkReply::NoError) {
        serverErrorCode = AIServer::networkReplyErrorToAiServerError(static_cast<QNetworkReply::NetworkError>(reply->error()));
        errorStr = reply->errorString();
    }

    reply->close();
    delete reply;
    reply = nullptr;

    jsonDocument = QJsonDocument::fromJson(replyData);
    QString responseContent;
    QJsonArray docArray;
    QJsonObject reference;
    reference.insert("type", 4);
    {
        QVariantHash values = jsonDocument.object().toVariantHash();
        responseContent = values.value("answer").toString();

        QVariantList refVar = values.value("references").toList();
        if (!refVar.isEmpty()) {
            QJsonObject item;
            item.insert("docPath", "AI招投标素材");

            {
                QJsonArray contens;
                for (const QVariant &var : refVar)
                    contens.append(var.toString());
                item.insert("docContents", contens);
            }
            docArray.append(item);
        }
    }
    reference.insert("sources", docArray);

    // run once event loop to ensure that client connected pipe.
    loop.processEvents();
    if (stream)
        stream(responseContent, user);

    if (responseContent.isEmpty())
        qWarning() << "response content is empty:" << url.toString();

    if (docArray.isEmpty())
        qWarning() << "response reference is empty:" << url.toString();

    QJsonObject response;
    response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
    response.insert(GENERATE_RESPONSE_CODE, serverErrorCode);
    response.insert(GENERATE_RESPONSE_ERRORMSG, errorStr);

    if (serverErrorCode == AIServer::ErrorType::NoError && !docArray.isEmpty())
        response.insert(GENERATE_RESPONSE_REFERENCES, reference);

    return response;
}

void ZtbLLM::setAbort()
{
    emit sigAbort();
}
