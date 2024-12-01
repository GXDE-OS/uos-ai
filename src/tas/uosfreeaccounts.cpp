#include "uosfreeaccounts.h"
#include "httpaccessmanager.h"
#include "httpeventloop.h"
#include "networkdefs.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QStandardPaths>

UosFreeAccounts::UosFreeAccounts()
{
    initServerAddress();
}

void UosFreeAccounts::initServerAddress()
{
    QString testServerJsonPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/TestServer.json";
    if (QFileInfo::exists(testServerJsonPath)) {
        QFile dataInfoFile(testServerJsonPath);
        if (dataInfoFile.open(QIODevice::ReadOnly)) {
            QByteArray jsonData = dataInfoFile.readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
            QJsonObject jsonObj = jsonDoc.object();
            m_serverAddress = jsonObj["TestServer"].toString();
            qInfo() << "API Server Address is Test.";
            return;
        }
    }
    m_serverAddress = AIServer::ServerAPIAddress;
}

UosFreeAccounts::~UosFreeAccounts()
{

}

UosFreeAccounts &UosFreeAccounts::instance()
{
    static UosFreeAccounts instance;
    return instance;
}

QNetworkReply::NetworkError UosFreeAccounts::freeAccountButtonDisplay(const QString &type, UosFreeAccountActivity &freeAccountActivity)
{
    QString url = m_serverAddress + QString("/getButtonDisplay?type=%1").arg(type);
    QNetworkReply::NetworkError error = QNetworkReply::NetworkError::NoError;

    QSharedPointer<HttpAccessmanager> httpAccessManager
        = QSharedPointer<HttpAccessmanager>(new HttpAccessmanager(""));
    QNetworkRequest req = httpAccessManager->baseNetWorkRequest(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (httpAccessManager != nullptr) {
        auto reply = httpAccessManager->get(req);
        HttpEventLoop loop(reply, "UosFreeAccounts::freeAccountButtonDisplay");
        loop.setHttpOutTime(15000);
        loop.exec();

        auto resp = getHttpResponse(httpAccessManager, &loop);

        auto &obj = resp.second;
        if (resp.first == QNetworkReply::NetworkError::NoError) {
            if (obj.contains("datas"))
                obj = obj["datas"].toObject();

            if (obj.contains("button_name_china"))
                freeAccountActivity.buttonNameChina = obj["button_name_china"].toString();
            if (obj.contains("button_name_english"))
                freeAccountActivity.buttonNameEnglish = obj["button_name_english"].toString();
            if (obj.contains("display"))
                freeAccountActivity.display = obj["display"].toInt();
            if (obj.contains("start_time"))
                freeAccountActivity.startTime = QDateTime::fromString(obj["start_time"].toString(), "yyyy-MM-dd hh:mm:ss");
            if (obj.contains("end_time"))
                freeAccountActivity.endTime = QDateTime::fromString(obj["end_time"].toString(), "yyyy-MM-dd hh:mm:ss");
            if (obj.contains("type"))
                freeAccountActivity.type = obj["type"].toString();
            if (obj.contains("url"))
                freeAccountActivity.url = obj["url"].toString();
        } else {
            if (obj.contains("status"))
                m_status = obj["status"].toInt();
            if (obj.contains("desc"))
                m_lastError = obj["desc"].toString();
            error = resp.first;
        }
    } else {
        error = QNetworkReply::NetworkError::UnknownServerError;
    }

    return error;
}


QNetworkReply::NetworkError UosFreeAccounts::getFreeAccount(const ModelType type, UosFreeAccount &freeAccount, int &status)
{
    status = 0;
    QString mac;
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    // mac地址正则表达
    static QRegularExpression regexEmpty("^((00[:-]){0,})00$");
    static QRegularExpression regexMac("^([0-9A-Fa-f]{2}[:-]){5,}([0-9A-Fa-f]{2})$");
    foreach (QNetworkInterface interface, interfaces) {
        if (interface.type() == QNetworkInterface::InterfaceType::Loopback) {
            continue;
        }
        mac = interface.hardwareAddress();
        // 过滤空mac地址
        if (mac.isEmpty() || regexEmpty.match(mac).hasMatch()) {
            continue;
        }
        // 过滤不符合格式要求的mac地址
        if (!regexMac.match(mac).hasMatch()) {
            continue;
        }
        break;
    }

    QJsonDocument document;
    QJsonObject sendJson;
    sendJson["mac"] = mac;
    sendJson["type"] = type;
    if (type == ModelType::FREE_KOL) {
        sendJson["appid"] = freeAccount.appid;
        sendJson["appkey"] = freeAccount.appkey;
        sendJson["appsecret"] = freeAccount.appsecret;
    }

    document.setObject(sendJson);
    const QByteArray &sendData = document.toJson(QJsonDocument::Compact);

    QString url = m_serverAddress + "/getFreeAccount";
    QNetworkReply::NetworkError error = QNetworkReply::NetworkError::NoError;

    QSharedPointer<HttpAccessmanager> httpAccessManager
        = QSharedPointer<HttpAccessmanager>(new HttpAccessmanager(""));
    QNetworkRequest req = httpAccessManager->baseNetWorkRequest(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (httpAccessManager != nullptr) {
        QNetworkReply *reply = httpAccessManager->post(req, sendData);
        HttpEventLoop loop(reply, "UosFreeAccounts::getFreeAccount");
        loop.setHttpOutTime(15000);
        loop.exec();

        auto resp = getHttpResponse(httpAccessManager, &loop);

        auto &obj = resp.second;
        if (resp.first == QNetworkReply::NetworkError::NoError) {
            if (obj.contains("datas"))
                obj = obj["datas"].toObject();

            if (obj.contains("type"))
                freeAccount.type = obj["type"].toInt();
            if (obj.contains("appid"))
                freeAccount.appid = obj["appid"].toString();
            if (obj.contains("appkey"))
                freeAccount.appkey = obj["appkey"].toString();
            if (obj.contains("appsecret"))
                freeAccount.appsecret = obj["appsecret"].toString();
            if (obj.contains("exp_time"))
                freeAccount.expTime = QDateTime::fromString(obj["exp_time"].toString(), "yyyy-MM-dd hh:mm:ss");
            if (obj.contains("start_time"))
                freeAccount.expTime = QDateTime::fromString(obj["start_time"].toString(), "yyyy-MM-dd hh:mm:ss");
            if (obj.contains("end_time"))
                freeAccount.expTime = QDateTime::fromString(obj["end_time"].toString(), "yyyy-MM-dd hh:mm:ss");
            if (obj.contains("use_limit"))
                freeAccount.useLimit = obj["use_limit"].toInt();
            if (obj.contains("has_used"))
                freeAccount.hasUsed = obj["has_used"].toInt();
            if (obj.contains("model"))
                freeAccount.llmModel = obj["model"].toInt();
            if (obj.contains("modelAddress"))
                freeAccount.modelUrl = obj["modelAddress"].toString();
        } else {
            if (obj.contains("status"))
                m_status = obj["status"].toInt();
            if (obj.contains("desc"))
                m_lastError = obj["desc"].toString();
            if (obj.contains("datas"))
                status = obj["datas"].toInt();

            error = resp.first;
        }

    } else {
        error = QNetworkReply::NetworkError::UnknownServerError;
    }

    return error;
}


QNetworkReply::NetworkError UosFreeAccounts::getDeterAccountLegal(const QString &appkey, int &available, QString &modelUrl)
{
    QJsonDocument document;
    QJsonObject sendJson;
    sendJson["appkey"] = appkey;
    document.setObject(sendJson);
    const QByteArray &sendData = document.toJson(QJsonDocument::Compact);

    QString url = m_serverAddress + "/deterAccountLegal";
    QNetworkReply::NetworkError error = QNetworkReply::NetworkError::NoError;

    QSharedPointer<HttpAccessmanager> httpAccessManager
        = QSharedPointer<HttpAccessmanager>(new HttpAccessmanager(""));
    QNetworkRequest req = httpAccessManager->baseNetWorkRequest(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    available = 0;
    if (httpAccessManager != nullptr) {
        QNetworkReply *reply = httpAccessManager->post(req, sendData);
        HttpEventLoop loop(reply, "UosFreeAccounts::getDeterAccountLegal");
        loop.setHttpOutTime(15000);
        loop.exec();

        auto resp = getHttpResponse(httpAccessManager, &loop);

        auto &obj = resp.second;
        if (resp.first == QNetworkReply::NetworkError::NoError) {
            if (obj.contains("datas"))
                available = obj["datas"].toInt();
            if (obj.contains("remark"))
                modelUrl = obj["remark"].toString();
        } else {
            if (obj.contains("status"))
                m_status = obj["status"].toInt();
            if (obj.contains("desc"))
                m_lastError = obj["desc"].toString();
            error = resp.first;
        }
    } else {
        error = QNetworkReply::NetworkError::UnknownServerError;
    }

    return error;
}

QNetworkReply::NetworkError UosFreeAccounts::increaseUse(const QString &appkey, int chatAction)
{
    QJsonDocument document;
    QJsonObject sendJson;
    sendJson["appkey"] = appkey;
    sendJson["useType"] = chatAction & ChatText2Image ? 1 : 0;
    document.setObject(sendJson);
    const QByteArray &sendData = document.toJson(QJsonDocument::Compact);

    QString url = m_serverAddress + "/increaseUse";
    QNetworkReply::NetworkError error = QNetworkReply::NetworkError::NoError;

    QSharedPointer<HttpAccessmanager> httpAccessManager
        = QSharedPointer<HttpAccessmanager>(new HttpAccessmanager(""));
    QNetworkRequest req = httpAccessManager->baseNetWorkRequest(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (httpAccessManager != nullptr) {
        QNetworkReply *reply = httpAccessManager->post(req, sendData);
        HttpEventLoop loop(reply, "UosFreeAccounts::increaseUse");
        loop.setHttpOutTime(15000);
        loop.exec();

        auto resp = getHttpResponse(httpAccessManager, &loop);
        auto &obj = resp.second;
        if (resp.first != QNetworkReply::NetworkError::NoError) {
            if (obj.contains("status"))
                m_status = obj["status"].toInt();
            if (obj.contains("desc"))
                m_lastError = obj["desc"].toString();
            error = resp.first;
        }
    } else {
        error = QNetworkReply::NetworkError::UnknownServerError;
    }

    return error;
}

QPair<QNetworkReply::NetworkError, QJsonObject> UosFreeAccounts::getHttpResponse(const QSharedPointer<HttpAccessmanager> hacc, const HttpEventLoop *loop)
{
    QNetworkReply::NetworkError netReplyError = QNetworkReply::NetworkError::NoError;
    QJsonObject obj = {};

    bool isAuthError = hacc->isAuthenticationRequiredError();
    if (isAuthError)
        netReplyError = QNetworkReply::NetworkError::AuthenticationRequiredError;
    else if (loop->getHttpStatusCode() == 429)
        netReplyError = QNetworkReply::NetworkError::InternalServerError;
    else
        netReplyError = loop->getNetWorkError();

    if (netReplyError == QNetworkReply::NetworkError::NoError) {
        QJsonDocument respJson = QJsonDocument::fromJson(loop->getHttpResult());
        if (respJson.isObject()) {
            obj = respJson.object();
            if (obj.contains("status")) {
                switch (obj["status"].toInt()) {
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
                    netReplyError = QNetworkReply::NetworkError::ServiceUnavailableError;
                    break;
                }
            }
        }
    }
    return QPair<QNetworkReply::NetworkError, QJsonObject>(netReplyError, obj);
}

QString UosFreeAccounts::getLastError() const
{
    return m_lastError;
}

int UosFreeAccounts::getErrorCode() const
{
    return m_status;
}
