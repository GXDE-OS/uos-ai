// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fydnnetwork.h"

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
using namespace uos_ai::fydn;

static const char *appID = "da97c3390d404c7ebe23c6554a85dcd1";
static const char *appKey = "136ceb0f46b9eeac3d02459ce8a6d397";
static const char *appSecret = "840ad9101aa2bb1bab8e31622b92b364";

FydnNetwork::FydnNetwork()
{

}

QPair<int, QString> FydnNetwork::request(const QJsonObject &data, const QString &urlPath)
{
    const QPair<int, QString> accessData = generateAccessToken();
    if (accessData.first != ErrorType::NoError)
        return accessData;

    QPair<int, QString> requestMsg;

    QJsonDocument jsonDocument(data);
    QString sendJsonStr = QString::fromUtf8(jsonDocument.toJson(QJsonDocument::Compact));
    QString postSignature = generateSignature(sendJsonStr, appKey);//签名生成时需要把键值升序排序，QJsonObject默认排序了？

    QNetworkRequest request(urlPath);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("FYDN-OP-RequestId", QUuid::createUuid().toString().toUtf8());
    request.setRawHeader("FYDN-OP-Sign", postSignature.toUtf8());
    request.setRawHeader("FYDN-OP-AccessToken", accessData.second.toUtf8());
    request.setRawHeader("FYDN-OP-AppID", appID);

    const QByteArray &sendData = jsonDocument.toJson(QJsonDocument::Compact);
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, sendData);
    QEventLoop loop;
    QTimer timer;
    QObject::connect(reply, &QNetworkReply::readyRead, this, [=, &timer, &requestMsg](){
        timer.start();

        const QByteArray &data = reply->readAll();
        QJsonObject obj = QJsonDocument::fromJson(data).object();
        if (obj.contains("code") && obj["code"].toInt() != 1000) {
            requestMsg = qMakePair(ErrorType::ModelError, obj["msg"].toString());
        } else {
            requestMsg = qMakePair(ErrorType::NoError, QString());
            emit FydnNetwork::sigReadStream(this->parseResultString(data));
        }
    });

    timer.setSingleShot(true);
    timer.setInterval(60 * 1000);
    timer.start();
    QObject::connect(&timer, &QTimer::timeout, this, [=, &loop, &requestMsg](){
        requestMsg = qMakePair(ErrorType::NetWorkError, QString());
        reply->abort();
        loop.quit();
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(this, &FydnNetwork::sigAbort, &loop, &QEventLoop::quit);
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

void FydnNetwork::setAbortRequest()
{
    emit FydnNetwork::sigAbort();
}

QString FydnNetwork::parseResultString(const QByteArray &result)
{
    QString parseRes;

    QRegularExpression regex(R"(data:\s*\{(.*)\})");
    QRegularExpressionMatchIterator iter = regex.globalMatch(result);

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString matchStr = match.captured(0);

        int startIndex = matchStr.indexOf('{');
        int endIndex = matchStr.lastIndexOf('}');

        if (startIndex < 0 || endIndex < startIndex)
            continue;

        QString resultContent = matchStr.mid(startIndex, endIndex - startIndex + 1);
        QJsonObject obj = QJsonDocument::fromJson(resultContent.toUtf8()).object();
        if (obj.isEmpty())
            continue;

        if (obj.contains("choices")) {
            QJsonObject choiceObj = obj["choices"].toArray()[0].toObject();
            if (choiceObj.contains("delta")) {
                QJsonObject deltaObj = choiceObj["delta"].toObject();
                if (deltaObj.contains("content")) {
                    QString content = deltaObj["content"].toString();
                    parseRes += content;
                }
            }
        }
    }

    return parseRes;
}

void FydnNetwork::onReadyRead()
{

}

QPair<int, QString> FydnNetwork::generateAccessToken() const
{
    QUrl url("https://api.cjbdi.com:8443/auth/api/token/getAccess");
    QUrlQuery query;
    query.addQueryItem("appKey", appKey);
    query.addQueryItem("appSecret", appSecret);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkAccessManager manager;
    QNetworkReply *tokenReply = manager.post(request, "");
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

    QJsonDocument replyJson = QJsonDocument::fromJson(tokenReply->readAll());
    QJsonObject obj;
    if (replyJson.isObject()) {
        obj = replyJson.object();
        if (obj["code"].toInt() != 1000) {
            return qMakePair(ErrorType::ModelError, obj["msg"].toString());
        }
    }
    tokenReply->deleteLater();
    return qMakePair(ErrorType::NoError, obj["data"].toObject()["accessKey"].toString());
}

QString FydnNetwork::generateSignature(const QString &jsonStr, const QString &appKey)
{
    QString signature = jsonStr;
    QByteArray signatureData = QCryptographicHash::hash((signature + appKey).toUtf8(), QCryptographicHash::Md5);
    return signatureData.toHex();
}
