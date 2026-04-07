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
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logTAS)

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
            qCWarning(logTAS) << "using test server:" << m_serverAddress;
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
            qCWarning(logTAS) << "Failed to get button display info, error:" << resp.first;
            if (obj.contains("status"))
                m_status = obj["status"].toInt();
            if (obj.contains("desc"))
                m_lastError = obj["desc"].toString();
            error = resp.first;
        }
    } else {
        qCCritical(logTAS) << "HttpAccessmanager is null";
        error = QNetworkReply::NetworkError::UnknownServerError;
    }

    return error;
}

QNetworkReply::NetworkError UosFreeAccounts::getFreeAccount(const ModelType type, const LLMChatModel &llm, UosFreeAccount &freeAccount, int &status)
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
    if (llm != NoModel) {
            sendJson["model"] = llm;
    }

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
                freeAccount.startTime = QDateTime::fromString(obj["start_time"].toString(), "yyyy-MM-dd hh:mm:ss");
            if (obj.contains("end_time"))
                freeAccount.endTime = QDateTime::fromString(obj["end_time"].toString(), "yyyy-MM-dd hh:mm:ss");
            if (obj.contains("use_limit"))
                freeAccount.useLimit = obj["use_limit"].toInt();
            if (obj.contains("has_used"))
                freeAccount.hasUsed = obj["has_used"].toInt();
            if (obj.contains("model"))
                freeAccount.llmModel = obj["model"].toInt();
            if (obj.contains("modelAddress"))
                freeAccount.modelUrl = obj["modelAddress"].toString();
        } else {
            qCWarning(logTAS) << "Failed to get free account, error:" << resp.first;
            if (obj.contains("status"))
                m_status = obj["status"].toInt();
            if (obj.contains("desc"))
                m_lastError = obj["desc"].toString();
            if (obj.contains("datas"))
                status = obj["datas"].toInt();

            error = resp.first;
        }

    } else {
        qCCritical(logTAS) << "HttpAccessmanager is null";
        error = QNetworkReply::NetworkError::UnknownServerError;
    }

    return error;
}

QNetworkReply::NetworkError UosFreeAccounts::getDeterAccountLegal(const QString &appkey, int &available, QString &modelUrl, bool &claimAgain)
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
            if (obj.contains("claimAgain"))
                claimAgain = obj["claimAgain"].toBool();  // 是否已领取额外额度 true: 已经领取，false: 未领取
        } else {
            qCWarning(logTAS) << "Failed to get deter account legal, error:" << resp.first;
            if (obj.contains("status"))
                m_status = obj["status"].toInt();
            if (obj.contains("desc"))
                m_lastError = obj["desc"].toString();
            error = resp.first;
        }
    } else {
        qCCritical(logTAS) << "HttpAccessmanager is null";
        error = QNetworkReply::NetworkError::UnknownServerError;
    }

    return error;
}

QNetworkReply::NetworkError UosFreeAccounts::claimAccountUsage(const QString &appkey, int &result, QString &msg)
{
    QJsonDocument document;
    QJsonObject sendJson;
    sendJson["appkey"] = appkey;
    document.setObject(sendJson);
    const QByteArray &sendData = document.toJson(QJsonDocument::Compact);

    QString url = m_serverAddress + "/account-limit";
    QNetworkReply::NetworkError error = QNetworkReply::NetworkError::NoError;

    QSharedPointer<HttpAccessmanager> httpAccessManager
        = QSharedPointer<HttpAccessmanager>(new HttpAccessmanager(""));
    QNetworkRequest req = httpAccessManager->baseNetWorkRequest(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (httpAccessManager != nullptr) {
        QNetworkReply *reply = httpAccessManager->put(req, sendData);
        HttpEventLoop loop(reply, "UosFreeAccounts::claimAccountUsage");
        loop.setHttpOutTime(15000);
        loop.exec();

        auto resp = getHttpResponse(httpAccessManager, &loop);

        auto &obj = resp.second;
        if (resp.first == QNetworkReply::NetworkError::NoError) {
            if (obj.contains("status"))
                result = obj["status"].toInt();
            if (obj.contains("desc"))
                msg = obj["desc"].toString();
        } else {
            qCWarning(logTAS) << "Failed to increase account limit, error:" << resp.first;
            if (obj.contains("status"))
                m_status = obj["status"].toInt();
            if (obj.contains("desc"))
                m_lastError = obj["desc"].toString();
            error = resp.first;
        }
    } else {
        qCCritical(logTAS) << "HttpAccessmanager is null";
        error = QNetworkReply::NetworkError::UnknownServerError;
    }

    return error;
}

QNetworkReply::NetworkError UosFreeAccounts::checkFreeModelActivity(const LLMChatModel type, int &result, UosFreeModelActivity &freeModelActivity)
{
    QString url = m_serverAddress + QString("/model-activity?model=%1").arg(type);
    QNetworkReply::NetworkError error = QNetworkReply::NetworkError::NoError;

    QSharedPointer<HttpAccessmanager> httpAccessManager
        = QSharedPointer<HttpAccessmanager>(new HttpAccessmanager(""));
    QNetworkRequest req = httpAccessManager->baseNetWorkRequest(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (httpAccessManager != nullptr) {
        auto reply = httpAccessManager->get(req);
        HttpEventLoop loop(reply, "UosFreeAccounts::checkFreeModelActivity");
        loop.setHttpOutTime(15000);
        loop.exec();

        auto resp = getHttpResponse(httpAccessManager, &loop);

        auto &obj = resp.second;
        if (resp.first == QNetworkReply::NetworkError::NoError) {
            if (obj.contains("status"))
                result = obj["status"].toInt();

            if (result != 200) { // 查询失败
                m_status = result;
                if (obj.contains("desc"))
                    m_lastError = obj["desc"].toString();

                error = QNetworkReply::NetworkError::ServiceUnavailableError;
                return error;
            }

            if (obj.contains("datas"))
                obj = obj["datas"].toObject();
            if (obj.contains("model"))
                freeModelActivity.type = obj["model"].toInt();
            if (obj.contains("inActivityPeriod"))
                freeModelActivity.inActivityPeriod = obj["inActivityPeriod"].toBool();
            if (obj.contains("startTime"))
                freeModelActivity.startTime = QDateTime::fromString(obj["startTime"].toString(), "yyyy-MM-dd hh:mm:ss");
            if (obj.contains("endTime"))
                freeModelActivity.endTime = QDateTime::fromString(obj["endTime"].toString(), "yyyy-MM-dd hh:mm:ss");
        } else {
            qCWarning(logTAS) << "Failed to check free model activity, error:" << resp.first;
            if (obj.contains("status"))
                m_status = obj["status"].toInt();
            if (obj.contains("desc"))
                m_lastError = obj["desc"].toString();
            error = resp.first;
        }
    } else {
        qCCritical(logTAS) << "HttpAccessmanager is null";
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
            qCWarning(logTAS) << "Failed to increase use, error:" << resp.first;
            if (obj.contains("status"))
                m_status = obj["status"].toInt();
            if (obj.contains("desc"))
                m_lastError = obj["desc"].toString();
            error = resp.first;
        }
    } else {
        qCCritical(logTAS) << "HttpAccessmanager is null";
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
                case 5001: // 领取额外额度：账户不存在
                case 5002: // 领取额外额度：仅支持DeepSeek账号领取额外额度
                case 5003: // 领取额外额度：账号额度尚未用完
                case 5004: // 领取额外额度：已参与过活动，无法重复领取
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
