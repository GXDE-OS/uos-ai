// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pptllm.h"

#include <QUrl>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QProcess>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QtDBus>

#define NM_SERVICE      "org.freedesktop.NetworkManager"
#define NM_PATH         "/org/freedesktop/NetworkManager"
#define NM_INTERFACE    "org.freedesktop.NetworkManager"

using namespace uos_ai;

enum NMState {
    NM_STATE_UNKNOWN = 0,
    NM_STATE_ASLEEP = 10,
    NM_STATE_DISCONNECTED = 20,
    NM_STATE_DISCONNECTING = 30,
    NM_STATE_CONNECTING = 40,
    NM_STATE_CONNECTED_LOCAL = 50,
    NM_STATE_CONNECTED_SITE = 60,
    NM_STATE_CONNECTED_GLOBAL = 70
};

static QByteArray generateHmac(QCryptographicHash::Algorithm algorithm, const QByteArray &data, const QByteArray &key)
{
    // 填充密钥以匹配块大小
    QByteArray keyPadded(64, 0x00);
    for (int i = 0; i < key.length(); ++i) {
        keyPadded[i] = key[i];
    }

    // 创建 oKeyPad 和 iKeyPad
    QByteArray oKeyPad = keyPadded;
    QByteArray iKeyPad = keyPadded;
    for (int i = 0; i < 64; ++i) {
        oKeyPad[i] = oKeyPad[i] ^ 0x5c;
        iKeyPad[i] = iKeyPad[i] ^ 0x36;
    }

    // 计算 HMAC
    QCryptographicHash hash(algorithm);
    hash.addData(iKeyPad);
    hash.addData(data);
    QByteArray innerHash = hash.result();

    hash.reset();
    hash.addData(oKeyPad);
    hash.addData(innerHash);
    return hash.result();
}

PPTLLM::PPTLLM() : QObject(), LLMModel()
{
    QNetworkAccessManager manager;
    // 设置请求的URL
    QUrl url("https://co.aiPPT.cn/api/grant/token");

    // 添加查询参数
    QUrlQuery query;
    query.addQueryItem("uid", "1");
    query.addQueryItem("channel", "");
    url.setQuery(query);

    QNetworkRequest request(url);
    QDateTime dateTime = QDateTime::currentDateTimeUtc();

    // 将时间转换为时间戳（自1970年1月1日以来的秒数）
    // 获取当前的Unix时间戳
    qint64 currentSeconds = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

    // 密钥和数据
    QByteArray timestamp = QString::number(currentSeconds).toUtf8();
    QByteArray key = "2YdWhPr6WYyWkPgZYQSuEILduOe4MvyI"; // 这里替换密钥
    // 构建需要签名的数据
    QByteArray data = "GET@/api/grant/token/@" + timestamp;

    QByteArray signature = generateHmac(QCryptographicHash::Sha1, data, key);

    // 设置自定义头
    request.setRawHeader("x-api-key", "6684d4c711462");
    request.setRawHeader("x-timestamp", timestamp);
    request.setRawHeader("x-signature", signature.toBase64());

    QEventLoop loop;
    // 发送请求
    QNetworkReply *reply = manager.get(request);
    // 等待请求完成
    QObject::connect(reply, &QNetworkReply::finished, [&]() {
        if (reply->error() == QNetworkReply::NoError) {
            // 请求成功，读取响应
            const QByteArray &data = reply->readAll();
            QJsonObject obj = QJsonDocument::fromJson(data).object();
            token = obj.value("data").toObject().value("token").toString();
        } else {
            // 请求失败，打印错误信息
            qInfo() << "Error:" << reply->errorString();
        }
        loop.quit();
        reply->deleteLater();
    });
    loop.exec();
}

QString PPTLLM::model() const
{
    return modelID();
}

QJsonObject PPTLLM::generate(const QString &content, const QVariantHash &params, LLMModel::streamFuncion stream, void *user)
{
    /* 1. 调用接口请求模型
     * 2. 流式接收数据通过stream(data, user); 非
     * 3. 流式添加到返回的Json对象的 “GENERATE_RESPONSE_CONTENT”字段中
     * 4. 组装回复JSON
     *      QJsonObject response;
            response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
            response.insert(GENERATE_RESPONSE_CODE, errorPair.first);
            response.insert(GENERATE_RESPONSE_ERRORMSG, errorPair.second);
     */

    QDBusMessage msg = QDBusMessage::createMethodCall(NM_SERVICE,
                                                      NM_PATH,
                                                      NM_INTERFACE,
                                                      "state");

    QDBusReply<quint32> ret = QDBusConnection::systemBus().call(msg);
    if (ret.isValid() && ret.value() == NM_STATE_CONNECTED_GLOBAL) {
        qCritical() << "network is online";
    } else {
        QEventLoop loop;
        QTimer::singleShot(1000, [&loop](){
            loop.quit();
        });
        loop.exec();
        QString responseContent;
        stream("连接失败，请检查是否连接了互联网，或稍后再试。", user);
        responseContent = "连接失败，请检查是否连接了互联网，或稍后再试。";
        QJsonObject response;
        response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
        response.insert(GENERATE_RESPONSE_CODE, 0);
        response.insert(GENERATE_RESPONSE_ERRORMSG, "");
        return response;
    }

    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    QJsonArray array = doc.array();
    QJsonValue value = array.last();
    QJsonObject obj = value.toObject();
    QString lastContent = obj["content"].toString();
    qInfo()<<"PPT Assistant content: "<<lastContent;
    QString wordFile = "";
    bool isDocEmpty = false;

    QJsonDocument contentDoc = QJsonDocument::fromJson(lastContent.toUtf8());
    // 确保JSON是对象类型
    if (contentDoc.isObject()) {
        QJsonObject contentObj = contentDoc.object();
        if (contentObj.contains("tagType")) {
            lastContent = contentObj["value"].toString();
            QEventLoop loop;
            QTimer::singleShot(1000, [&loop](){
                loop.quit();
            });
            loop.exec();
            QString responseContent;
            if (lastContent.trimmed().isEmpty()) {
                //指令内容为空，同时上传空白文件时
                stream("文件为空，请更换文件。", user);
                responseContent = "文件为空，请更换文件。";
            } else {
                stream("PPT大纲为：\n", user);
                stream(lastContent, user);
                responseContent = "PPT大纲为：" + lastContent;
            }
            QJsonObject response;
            response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
            response.insert(GENERATE_RESPONSE_CODE, 0);
            response.insert(GENERATE_RESPONSE_ERRORMSG, "");
            return response;
        } else if(contentObj.contains("docPath")) {
            wordFile = contentObj["docPath"].toString();
            isDocEmpty = contentObj["isDocEmpty"].toBool();
        }
    }

    QNetworkAccessManager manager;
    QUrl url("https://co.aiPPT.cn/api/ai/chat/v2/task");
    QNetworkRequest request(url);
    request.setRawHeader("x-api-key", "6684d4c711462");
    request.setRawHeader("x-channel", "");
    request.setRawHeader("x-token", token.toUtf8());

    // 创建 QHttpMultiPart 对象
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // 添加文本字段
    if (wordFile == "") {
        QHttpPart contentPart;
        contentPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"content\"")));
        contentPart.setBody(lastContent.toUtf8()); // 替换为实际内容
        multiPart->append(contentPart);
    }

    QHttpPart idPart;
    idPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"id\"")));
    idPart.setBody(""); // 替换为实际 ID
    multiPart->append(idPart);

    QHttpPart titlePart;
    titlePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"title\"")));
    titlePart.setBody(""); // 替换为实际标题
    multiPart->append(titlePart);

    QHttpPart typePart;  
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"type\"")));
    if (wordFile == "") {
        typePart.setBody("11");
    } else {
        typePart.setBody("3");
    }
    multiPart->append(typePart);

    //    添加文件字段
    if (wordFile != "") {
        QHttpPart filePart;
        QFile *file = new QFile(wordFile);
        if (!file->open(QIODevice::ReadOnly)) {
            QEventLoop loop;
            QTimer::singleShot(1000, [&loop](){
                loop.quit();
            });
            loop.exec();
            QString responseContent;
            stream("文件不存在，请更换文件。", user);
            responseContent = "文件不存在，请更换文件。";
            QJsonObject response;
            response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
            response.insert(GENERATE_RESPONSE_CODE, 0);
            response.insert(GENERATE_RESPONSE_ERRORMSG, "");
            return response;
        }
        if (isDocEmpty) {
            QEventLoop loop;
            QTimer::singleShot(1000, [&loop](){
                loop.quit();
            });
            loop.exec();
            QString responseContent;
            stream("文件为空，请更换文件。", user);
            responseContent = "文件为空，请更换文件。";
            QJsonObject response;
            response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
            response.insert(GENERATE_RESPONSE_CODE, 0);
            response.insert(GENERATE_RESPONSE_ERRORMSG, "");
            return response;
        }
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"file\"; filename=\"%1\"").arg(file->fileName())));
        filePart.setBodyDevice(file);
        file->setParent(multiPart); // 文件随 QHttpMultiPart 一起删除
        multiPart->append(filePart);
    }

    QNetworkReply *tokenReply = manager.post(request, multiPart);
    multiPart->setParent(tokenReply); // 确保 QHttpMultiPart 随 QNetworkReply 一起删除
    QEventLoop loop;
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QString responseContent;
    if (tokenReply->error() == QNetworkReply::NoError) {
        const QByteArray &data = tokenReply->readAll();
        QJsonObject obj = QJsonDocument::fromJson(data).object();
        if (obj.value("code").toInt() == 0) {
            int task_id = obj.value("data").toObject().value("id").toInt();
            responseContent = getContent(task_id, stream, user);
        } else {
            stream(obj.value("msg").toString(), user);
            QString responseContent = obj.value("msg").toString();
        }
    }

    QJsonObject response;
    response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
    response.insert(GENERATE_RESPONSE_CODE, 0);
    response.insert(GENERATE_RESPONSE_ERRORMSG, "");
    return response;
}

QString PPTLLM::getContent(int id, LLMModel::streamFuncion stream, void *user)
{
    QNetworkAccessManager manager;
    QUrl url("https://co.aippt.cn/api/ai/chat/v2/word");
    QUrlQuery query;
    query.addQueryItem("task_id", QString::number(id)); // 替换为实际的任务 ID
    url.setQuery(query);

    // 创建网络请求对象
    QNetworkRequest request(url);

    // 设置请求头
    request.setRawHeader("x-api-key", "6684d4c711462");
    request.setRawHeader("x-channel", "");
    request.setRawHeader("x-token", token.toUtf8());

    // 发送 GET 请求
    QNetworkReply *contentReply = manager.get(request);

    // 等待请求完成
    QString content_result ="";
    bool startStream = false;
    bool endStream = false;
    QEventLoop loop;
    connect(this, &PPTLLM::sigAbort, this, [&](){
        contentReply->abort();
        content_result = "task cancel";
    });
    QObject::connect(contentReply, &QNetworkReply::readyRead, [&]() {
        if (contentReply->error() == QNetworkReply::NoError) {
            QString text = QString::fromUtf8(contentReply->readAll());
            // 解析事件流
            QStringList lines = text.split("\n");
            for (const QString& line : lines) {
                if (line.startsWith("data:")) {
                    QString jsonStr = line.mid(5); // Remove "data:" prefix
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
                    if (!jsonDoc.isNull()) {
                        QJsonObject jsonObj = jsonDoc.object();
                        if (jsonObj.contains("content")) {
                            QString content = jsonObj.value("content").toString();
                            bool firstLine = content.contains("#");
                            if (!startStream && firstLine) {
                                startStream = true;
                                stream("PPT大纲为：", user);
                                content_result += "PPT大纲为：";
                            }
                            if (startStream && !endStream){
                                if (content.contains("`")) {
                                    int backtickIndex = content.indexOf("`");
                                    if (backtickIndex != -1) {
                                        // 如果找到了反引号，去掉反引号之后的所有字符
                                        content = content.left(backtickIndex);
                                        endStream = true;
                                    }
                                }
                                stream(content, user);
                                content_result += content;
                            }
                        }
                    }
                }
            }
        } else {
            qInfo() << "Error:" << contentReply->errorString();
        }
    });
    QObject::connect(contentReply, &QNetworkReply::finished, [&]() {
        if (contentReply->error() == QNetworkReply::NoError) {
            QString text = QString::fromUtf8(contentReply->readAll());

            // 解析事件流
            QStringList lines = text.split("\n");
            for (const QString& line : lines) {
                if (line.startsWith("data:")) {
                    QString jsonStr = line.mid(5); // Remove "data:" prefix
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
                    if (!jsonDoc.isNull()) {
                        QJsonObject jsonObj = jsonDoc.object();
                        if (jsonObj.contains("content")) {
                            QString content = jsonObj.value("content").toString();
                            stream(content, user);
                            content_result += content;
                        }
                    }
                }
            }
        } else {
            qInfo() << "Error:" << contentReply->errorString();
        }
        loop.quit();
        contentReply->deleteLater();
    });
    loop.exec();
    return content_result;
}

void PPTLLM::setAbort()
{
    emit sigAbort();
}
