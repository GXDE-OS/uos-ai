#include "uossimplelog.h"
#include "httpaccessmanager.h"
#include "httpeventloop.h"
#include "networkdefs.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>

QMap<UosLogType, QString> gServerUrls = {
    {UosLogType::UserInput, "https://uosai.uniontech.com/api/saveAiModel"},
    {UosLogType::FailedRetry, "https://uosai.uniontech.com/api/saveRetryRecord"},
    {UosLogType::TextToImageResult, "https://uosai.uniontech.com/api/saveModelResult"}
};

UosSimpleLog::UosSimpleLog(QObject *parent)
    : QThread(parent)
    , m_stopLogging(false)
{
    start();
}

UosSimpleLog::~UosSimpleLog()
{
    m_stopLogging.store(true);
    while (isRunning()) {
        m_condition.wakeAll();
        QThread::msleep(200);
    }
}

UosSimpleLog &UosSimpleLog::instance()
{
    static UosSimpleLog instance(nullptr);
    return instance;
}

void UosSimpleLog::addLog(const UosLogObject &logObj)
{
    if (!m_stopLogging.load()) {
        m_preLogObjectQueue.enqueue(logObj);
        m_mutex.lock();
        m_condition.wakeAll();
        m_mutex.unlock();
    }
}

void UosSimpleLog::run()
{
    while (!m_stopLogging.load()) {
        m_mutex.lock();
        if (m_preLogObjectQueue.isEmpty()) {
            m_condition.wait(&m_mutex, 1000);
        }
        m_mutex.unlock();

        UosLogObject logObj;
        while (m_preLogObjectQueue.dequeue(logObj) && !m_stopLogging.load()) {
            int nCount = 3;
            while (nCount-- > 0) {
                if (QNetworkReply::NetworkError::NoError != pushLog(logObj)) {
                    qWarning() << "Try push log again.";
                } else {
                    break;
                }
            }
        }
    }
}

int UosSimpleLog::pushLog(const UosLogObject &logObj)
{
    QJsonDocument document;
    QJsonObject logTextJson;
    logTextJson["app"] = logObj.app;
    logTextJson["llm"] = logObj.llm;

    if (logObj.type == UosLogType::UserInput || logObj.type == UosLogType::TextToImageResult) {
        logTextJson["content"] = logObj.content;
        logTextJson["datetime"] = logObj.time.toString("yyyy-MM-dd hh:mm:ss");;
        logTextJson["modelType"] = logObj.ModelType;
    }

    if (logObj.type == UosLogType::TextToImageResult) {
        logTextJson["result"] = logObj.t2iResult;
    }

    document.setObject(logTextJson);

    const QByteArray &sendData = document.toJson(QJsonDocument::Compact);

    QSharedPointer<HttpAccessmanager> httpAccessManager
        = QSharedPointer<HttpAccessmanager>(new HttpAccessmanager(""));
    QNetworkRequest req = httpAccessManager->baseNetWorkRequest(QUrl(hostUrl(logObj.type)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = httpAccessManager->post(req, sendData);

    HttpEventLoop loop(reply, "UosSimpleLog::pushLog");
    loop.setHttpOutTime(15000);
    loop.exec();

    QNetworkReply::NetworkError netReplyError = QNetworkReply::NetworkError::UnknownServerError;
    bool isAuthError = httpAccessManager->isAuthenticationRequiredError();
    if (isAuthError)
        netReplyError = QNetworkReply::NetworkError::AuthenticationRequiredError;
    else if (loop.getHttpStatusCode() == 429)
        netReplyError = QNetworkReply::NetworkError::InternalServerError;
    else
        netReplyError = loop.getNetWorkError();

    if (netReplyError == QNetworkReply::NetworkError::NoError) {
        QJsonDocument respJson = QJsonDocument::fromJson(loop.getHttpResult());
        if (respJson.isObject()) {
            auto resultObj = respJson.object();
            if (resultObj.contains("status")) {
                switch (resultObj["status"].toInt()) {
                case 200:
                    netReplyError = QNetworkReply::NetworkError::NoError;
                    break;
                case 201:
                    netReplyError = QNetworkReply::NetworkError::OperationNotImplementedError;
                    break;
                case 401:
                    netReplyError = QNetworkReply::NetworkError::AuthenticationRequiredError;
                    break;
                case 403:
                    netReplyError = QNetworkReply::NetworkError::ContentAccessDenied;
                    break;
                case 404:
                    netReplyError = QNetworkReply::NetworkError::HostNotFoundError;
                    break;
                default:
                    break;
                }
            }
        }
    }
    if (netReplyError != QNetworkReply::NetworkError::NoError) {
        qWarning() << "push log failed: \n\tReq: " << sendData.toStdString().c_str()
                   << " \n\tResp: " << loop.getHttpResult().toStdString().c_str();
    }

    return netReplyError;
}

QString UosSimpleLog::simplifiedText(const QString &content)
{
    int start = content.indexOf("Input:") + strlen("Input:");
    int end = content.indexOf("Output:");
    if (start < 0 || (end < start)) {
        return content;
    }
    return QString(content.begin() + start
                   , (end < 0 ? content.length() - start : end - start))
           .simplified();
}

QString UosSimpleLog::hostUrl(UosLogType type) const
{
    if (gServerUrls.contains(type))
        return gServerUrls[type];
    return "";
}
