// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tzdlllm.h"

#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>

using namespace uos_ai;
using namespace tzdl;

// 在文件头部添加
#include <QLoggingCategory>
Q_LOGGING_CATEGORY(logTzdl, "uosai.tzdl")

// 重写构造函数，传入当前是第几个配置文件，用于生成不同的配置文件名
TzdlLLM::TzdlLLM(QString configFileName, TzdlConfig config, QObject *parent)
    : QObject(parent), LLMModel(),
      m_serverRootUrl(config.serverRootUrl),
      m_agentCode(config.agentCode),
      m_agentVersion(config.agentVersion),
      m_tokenID(config.tokenID),
      m_createSessionRoute(config.createSessionRoute),
      m_runSessionRoute(config.runSessionRoute),
      m_clearSessionRoute(config.clearSessionRoute)
{
    m_configFileName = configFileName;
}

QString TzdlLLM::model() const
{
    return QString("Tzdl LLM_" + m_configFileName);
}

// 修改 generate() 函数中的日志
QJsonObject TzdlLLM::generate(const QString &content, const QVariantHash &params, LLMModel::streamFuncion stream, void *user)
{
    QString responseContent;
    QJsonObject sendObj;
    QJsonArray conversions = QJsonDocument::fromJson(content.toUtf8()).array();
    QString question;
    QJsonArray history;
    if (!conversions.isEmpty())
    {
        QJsonObject obj = conversions.takeAt(conversions.size() - 1).toObject();
        question = obj.value("content").toString();

        QJsonArray qa;
        for (const QJsonValue &tmp : conversions)
        {
            qa.append(tmp.toObject().value("content").toString());
            if (qa.size() == 2)
            {
                history.append(qa);
                qa = QJsonArray();
            }
        }
    }

    QUrl url;
    QEventLoop loop;
    AIServer::ErrorType serverErrorCode;
    QByteArray replyData;
    QString errorStr;

    // 创建session的请求数据结构
    QJsonObject createSessionObj;
    createSessionObj.insert("agentCode", m_agentCode);
    createSessionObj.insert("agentVersion", m_agentVersion);
    QJsonDocument createSessionJsonDocument(createSessionObj);
    QByteArray createSessionSendData = createSessionJsonDocument.toJson(QJsonDocument::Compact);
    QString urlCreateSesion = m_serverRootUrl + m_createSessionRoute;
    url = QUrl(urlCreateSesion);
    replyData = httpRequest(url, createSessionSendData, loop, serverErrorCode, errorStr, responseContent, true);

    qCDebug(logTzdl) << "Create session response:" << replyData;

    QJsonDocument sessionReplyDoc = QJsonDocument::fromJson(replyData);
    QJsonObject sessionReplyObj = sessionReplyDoc.object();

    QString uniqueCode;
    if (sessionReplyObj.value("success").toBool())
    {
        QJsonObject dataObj = sessionReplyObj.value("data").toObject();
        uniqueCode = dataObj.value("uniqueCode").toString();
        qCDebug(logTzdl) << "Session created, uniqueCode:" << uniqueCode;
    }
    else
    {
        qCWarning(logTzdl) << "Failed to create session, response:" << sessionReplyObj;
    }

    // 发起agent调用
    if (!uniqueCode.isEmpty())
    {
        QJsonObject agentCallObj;
        agentCallObj.insert("sessionId", uniqueCode);
        agentCallObj.insert("stream", true); // 是否流式，默认true
        agentCallObj.insert("delta", true);  // 是否增量，默认true
        agentCallObj.insert("trace", false); // 是否追踪，默认false

        // 构建message对象
        QJsonObject messageObj;
        messageObj.insert("text", question);
        agentCallObj.insert("message", messageObj);

        // 构建空的metadata和attachments
        agentCallObj.insert("metadata", QJsonObject());
        agentCallObj.insert("attachments", QJsonArray());

        QJsonDocument agentJsonDocument(agentCallObj);
        QByteArray agentSendData = agentJsonDocument.toJson(QJsonDocument::Compact);

        QString agentUrl = m_serverRootUrl + m_runSessionRoute;
        QUrl agentCallUrl(agentUrl);
        QByteArray agentReplyData = httpRequest(agentCallUrl, agentSendData, loop, serverErrorCode, errorStr, responseContent, true, true, stream, user);

        qCDebug(logTzdl) << "Agent response content:" << responseContent;
    }

    // 清理session
    if (!uniqueCode.isEmpty())
    {
        QJsonObject clearSessionObj;
        clearSessionObj.insert("sessionId", uniqueCode);
        QJsonDocument clearSessionJsonDocument(clearSessionObj);
        QByteArray clearSessionSendData = clearSessionJsonDocument.toJson(QJsonDocument::Compact);
        QString urlClearSesion = m_serverRootUrl + m_clearSessionRoute;
        url = QUrl(urlClearSesion);
        replyData = httpRequest(url, clearSessionSendData, loop, serverErrorCode, errorStr, responseContent, true);

        qCDebug(logTzdl) << "Clear session response:" << replyData;
    }

    // run once event loop to ensure that client connected pipe.
    loop.processEvents();

    if (responseContent.isEmpty())
        qCWarning(logTzdl) << "Empty response content from:" << url.toString();

    QJsonObject response;
    response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
    response.insert(GENERATE_RESPONSE_CODE, serverErrorCode);
    response.insert(GENERATE_RESPONSE_ERRORMSG, errorStr);

    qCDebug(logTzdl) << "Final response object:" << response;
    return response;
}

void TzdlLLM::setAbort()
{
    emit sigAbort();
}

QByteArray TzdlLLM::httpRequest(QUrl url,
                                    QByteArray sendData,
                                    QEventLoop &loop,
                                    AIServer::ErrorType &serverErrorCode,
                                    QString &errorStr,
                                    QString &responseContent,
                                    bool needAuth,
                                    bool isAgentCall,
                                    LLMModel::streamFuncion stream,
                                    void *user)
{
    QByteArray replyData;

    // 打印发送的数据
    qCDebug(logTzdl) << "Sending request to URL:" << url.toString();
    qCDebug(logTzdl) << "Send data:" << QString::fromUtf8(sendData);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 如果需要认证，添加Authorization头
    if (needAuth)
    {
        QString authHeader = "Bearer " + m_tokenID;
        request.setRawHeader("Authorization", authHeader.toUtf8());
    }

    QNetworkAccessManager manager;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(5 * 60 * 1000);

    QNetworkReply *reply = manager.post(request, sendData);

    QObject::connect(reply, &QNetworkReply::readyRead, this, [=, &timer, &replyData, &responseContent]()
                     {
        timer.start();
        QByteArray replyData_ = reply->readAll();
        qCDebug(logTzdl) << "Received response data:" << replyData_;
        replyData += replyData_;
        if(isAgentCall){
            QString ret = parseContentString(replyData_, stream, user);
            responseContent += ret;
        } });

    timer.start();

    serverErrorCode = AIServer::ErrorType::NoError;
    QObject::connect(&timer, &QTimer::timeout, this, [=, &loop, &serverErrorCode]()
                     {
        serverErrorCode = AIServer::ErrorType::TimeoutError;
        reply->abort();
        loop.quit(); });

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(this, &TzdlLLM::sigAbort, reply, &QNetworkReply::abort);
    QObject::connect(this, &TzdlLLM::sigAbort, &loop, &QEventLoop::quit);

    loop.exec();
    timer.stop();

    if (serverErrorCode == AIServer::ErrorType::NoError && reply->error() != QNetworkReply::NoError)
    {
        serverErrorCode = AIServer::networkReplyErrorToAiServerError(static_cast<QNetworkReply::NetworkError>(reply->error()));
        errorStr = reply->errorString();
    }

    reply->close();
    delete reply;
    reply = nullptr;

    return replyData;
}

QString TzdlLLM::parseContentString(const QByteArray &content, LLMModel::streamFuncion stream, void *user)
{
    QString contentStr = QString::fromUtf8(content);
    qCDebug(logTzdl) << "Original content:" << contentStr;
    
    // 去掉"data:"前缀
    if (contentStr.startsWith("data:")) {
        contentStr = contentStr.mid(5); // 去掉"data:"前缀
        qCDebug(logTzdl) << "Content after removing 'data:' prefix:" << contentStr;
    }
    
    // 检查是否为有效的JSON字符串
    QJsonParseError parseError;
    QJsonDocument agentReplyDoc = QJsonDocument::fromJson(contentStr.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(logTzdl) << "JSON parse error:" << parseError.errorString() << "at position:" << parseError.offset;
        qCWarning(logTzdl) << "Invalid JSON content:" << contentStr;
        return QString();
    }
    
    qCDebug(logTzdl) << "Agent reply document:" << agentReplyDoc;
    QJsonObject agentReplyObj = agentReplyDoc.object();

    QString objectType = agentReplyObj.value("object").toString();
    qCDebug(logTzdl) << "Stream object type:" << objectType;

    QString contentText;
    QJsonObject reasoningObj;
    if (objectType == "message.delta")
    {
        QJsonArray contentArray = agentReplyObj.value("content").toArray();
        QString text;
        for (const QJsonValue &contentValue : contentArray) {
            QJsonObject contentObj = contentValue.toObject();
            if (contentObj.value("type").toString() == "text") {
                text += contentObj.value("text").toObject().value("value").toString();
            }
        }
        contentText = text;
        reasoningObj.insert("content", contentText);
        qCDebug(logTzdl) << "Received message delta content" << contentText;
    }
    else if (objectType == "thought.delta")
    {
        QJsonObject contentObj = agentReplyObj.value("content").toObject();
        contentText = contentObj.value("data").toString();
        reasoningObj.insert("reasoningContent", contentText);
        qCDebug(logTzdl) << "Received thought delta content"  << contentText;
    }
    else if (objectType == "error")
    {
        QJsonObject contentObj = agentReplyObj.value("content").toObject();
        
        // 获取errorMsg字符串
        QString errorMsgStr = contentObj.value("errorMsg").toString();
        qCDebug(logTzdl) << "Error message string:" << errorMsgStr;
        
        // 解析errorMsg中的JSON字符串
        QJsonParseError errorMsgParseError;
        QJsonDocument errorMsgDoc = QJsonDocument::fromJson(errorMsgStr.toUtf8(), &errorMsgParseError);
        
        if (errorMsgParseError.error == QJsonParseError::NoError) {
            QJsonObject errorMsgObj = errorMsgDoc.object();
            qCDebug(logTzdl) << "Parsed errorMsg object:" << errorMsgObj;
            QString textObj = errorMsgObj.value("text").toString().toUtf8();
            qCDebug(logTzdl) << "Text object:" << textObj;
            contentText = textObj;
        } else {
            qCWarning(logTzdl) << "Failed to parse errorMsg JSON:" << errorMsgParseError.errorString();
            qCWarning(logTzdl) << "Error message content:" << errorMsgStr;
            // 如果解析失败，使用原始errorMsg字符串
            contentText = errorMsgStr;
        }
        
        reasoningObj.insert("content", contentText);
        qCWarning(logTzdl) << "Received error response:" << agentReplyObj;
    }
    else
    {
        qCWarning(logTzdl) << "Unknown response type:" << objectType;
    }

    // 将JSON对象转换为JSON字符串
    QJsonDocument reasoningDoc(reasoningObj);
    QString reasoningJsonString = reasoningDoc.toJson(QJsonDocument::Compact);

    stream(reasoningJsonString, user);
    return contentText;
}
