// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rpanetwork.h"
#include "oafunctionhandler.h"

#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QTimer>
#include <QRegularExpression>
#include <QDebug>

using namespace uos_ai;
using namespace uos_ai::rpa;

static const char *appId = "38602585";
static const char *apiKey = "KBGOQ6tjAWduDhdMKcOxNuu6";
static const char *apiSecret = "POLypLxsZmpxbBZPQZqrtfGCubk0havQ";

static const char *replyContent = "content";
static const char *replyFunction = "tools";

RpaNetwork::RpaNetwork()
{

}

QPair<int, QString> RpaNetwork::request(const QJsonObject &data, const QString &urlPath)
{
    static QString accessToken;
    static QString accountId;

    QString id = QString(appId) + QString(apiKey) + QString(apiSecret);
    if (accessToken.isEmpty() || accountId != id) {
        const QPair<int, QString> &resultTokens = generateAccessToken(apiKey, apiSecret);
        if (resultTokens.first != QNetworkReply::NoError) {
            return resultTokens;
        }
        accountId = id;
        accessToken = resultTokens.second;
    }

    QUrl url(urlPath);
    QUrlQuery query;
    query.addQueryItem("access_token", accessToken);
    url.setQuery(query);

    const QByteArray &sendData = QJsonDocument(data).toJson(QJsonDocument::Compact);
    QPair<int, QString> requestMsg;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, sendData);
    QEventLoop loop;
    QTimer timer;
    QObject::connect(reply, &QNetworkReply::readyRead, this, [=, &timer, &requestMsg](){
        timer.start();

        const QByteArray &data = reply->readAll();
        QJsonObject obj = QJsonDocument::fromJson(data).object();
        if (obj.contains("error_code") && obj["error_code"].toInt() > 0) {
            requestMsg = qMakePair(ErrorType::ModelError, obj["error_msg"].toString());
        } else {
            requestMsg = qMakePair(ErrorType::NoError, QString());
            QString dataStr = QString::fromUtf8(data);
            QJsonObject replyObj = this->parserContent(dataStr);
            qInfo() << replyObj;
            if (replyObj.contains(replyContent)) {
                // textplain
                emit RpaNetwork::sigReadStream(replyObj.value(replyContent).toString());
            } else if (replyObj.contains(replyFunction)) {
                QJsonObject funcObj = replyObj.value(replyFunction).toObject().value("function_call").toObject();
                // FC process
                QString res = OaFunctionHandler::instance()->OaFunctionCall(funcObj);
                emit RpaNetwork::sigReadStream(res);
            }
        }
    });

    timer.setSingleShot(true);
    timer.setInterval(180 * 1000);
    timer.start();
    QObject::connect(&timer, &QTimer::timeout, this, [=, &loop, &requestMsg](){
        requestMsg = qMakePair(ErrorType::NetWorkError, QString());
        reply->abort();
        loop.quit();
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(this, &RpaNetwork::sigAbort, &loop, &QEventLoop::quit);
    loop.exec();
    timer.stop();

    // network reply error
    if (reply->error() != QNetworkReply::NoError) {
        return qMakePair(ErrorType::NetWorkError, reply->errorString());
    }

    reply->close();
    reply->deleteLater();
    reply = nullptr;

    return requestMsg;
}

void RpaNetwork::setAbortRequest()
{
    emit RpaNetwork::sigAbort();
}

void RpaNetwork::onReadyRead()
{

}

QPair<int, QString> RpaNetwork::generateAccessToken(const QString &clientId, const QString &clientSecret) const
{
    QUrl url("https://aip.baidubce.com/oauth/2.0/token");
    QUrlQuery query;
    query.addQueryItem("grant_type", "client_credentials");
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("client_secret", clientSecret);
    url.setQuery(query);

    QNetworkRequest accessRequest(url);
    accessRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    accessRequest.setRawHeader("Accept", "text/event-stream");

    QNetworkAccessManager manager;
    QNetworkReply *tokenReply = manager.post(accessRequest, "");
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(15000);
    timer.start();
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (tokenReply->error() != QNetworkReply::NoError) {
        return qMakePair(ErrorType::NetWorkError, tokenReply->errorString());
    }

    QJsonObject replyObj = QJsonDocument::fromJson(tokenReply->readAll()).object();
    QString accessToken = replyObj.value("access_token").toString();

    tokenReply->deleteLater();
    return qMakePair(ErrorType::NoError, accessToken);
}

QJsonObject RpaNetwork::parserContent(const QString &content)
{
    if (!content.trimmed().endsWith("}")) {
        return QJsonObject();
    }

    QRegularExpression regex(R"(data:\s*\{(.*)\})");
    QRegularExpressionMatchIterator iter = regex.globalMatch(content);

    QByteArray cacheDeltacontent;
    QMap<int, QString> seqContents;

    QJsonObject functionCall;
    QMap<int, QJsonObject> toolCallMaps;

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString matchString = match.captured(0);

        int startIndex = matchString.indexOf('{');
        int endIndex = matchString.lastIndexOf('}');

        if (startIndex >= 0 && endIndex > startIndex) {
            QString content = matchString.mid(startIndex, endIndex - startIndex + 1);

            QJsonObject j = QJsonDocument::fromJson(content.toUtf8()).object();
            if (j.isEmpty()) {
                cacheDeltacontent += matchString.toUtf8();
            } else {
                if (j.contains("result")) {
                    seqContents[j.value("sentence_id").toInt()] += j.value("result").toString();
                }

                if (j.contains("function_call")) {
                    const QJsonObject &function_call =  j.value("function_call").toObject();
                    if (function_call.contains("name")) {
                        functionCall["name"] = functionCall["name"].toString() + function_call.value("name").toString();
                    }

                    if (function_call.contains("arguments")) {
                        functionCall["arguments"] = functionCall["arguments"].toString() + function_call.value("arguments").toString();
                    }
                }

                if (j.contains("tool_calls")) {
                    const QJsonArray &tool_calls =  j.value("tool_calls").toArray();
                    for (const QJsonValue &tool_call : tool_calls) {
                        const QJsonObject &toolCallObj = tool_call.toObject();

                        int index = toolCallObj["index"].toInt();
                        if (!toolCallMaps[index].contains("function")) {
                            toolCallMaps[index]["function"] = QJsonObject();
                        }

                        toolCallMaps[index]["index"] = index;

                        if (toolCallObj.contains("id")) {
                            toolCallMaps[index]["id"] = toolCallObj.value("id");
                        }

                        if (toolCallObj.contains("type")) {
                            toolCallMaps[index]["type"] = toolCallObj.value("type");
                        }

                        if (toolCallObj.contains("function")) {
                            QJsonObject toolFun = toolCallMaps[index]["function"].toObject();
                            const QJsonObject &tmpToolFun =  toolCallObj.value("function").toObject();
                            if (tmpToolFun.contains("name")) {
                                toolFun["name"] = toolFun["name"].toString() + tmpToolFun.value("name").toString();
                            }

                            if (tmpToolFun.contains("arguments")) {
                                toolFun["arguments"] = toolFun["arguments"].toString() + tmpToolFun.value("arguments").toString();
                            }

                            toolCallMaps[index]["function"] = toolFun;
                        }
                    }
                }
            }
        }
    }

    QString deltacontent;
    for (auto iter = seqContents.begin(); iter != seqContents.end(); iter++) {
        deltacontent += iter.value();
    }

    QJsonObject response;
    if (!deltacontent.isEmpty()) {
        response[replyContent] = deltacontent;
    }

    QJsonObject tools;
    if (!functionCall.isEmpty()) {
        tools["function_call"] = functionCall;
        response[replyFunction] = tools;
    }

    QJsonArray toolCalls;
    for (auto iter = toolCallMaps.begin(); iter != toolCallMaps.end(); iter++) {
        toolCalls << iter.value();
    }

    if (!toolCalls.isEmpty()) {
        tools["tool_calls"] = toolCalls;
        response[replyFunction] = tools;
    }

    return response;
}
