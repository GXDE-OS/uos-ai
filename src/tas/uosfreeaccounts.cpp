#include "uosfreeaccounts.h"
#include "network/httpclient.h"
#include "global_define.h"

#include <QFileInfo>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QDateTime>

Q_DECLARE_LOGGING_CATEGORY(logTAS)
using namespace uos_ai;

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
    m_serverAddress = UosAPIAddress;
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
    
    HttpClient httpClient;
    httpClient.setTimeOut(60000);
    QVariantHash header;
    header["Content-Type"] = "application/json";
    
    auto response = httpClient.get(QUrl(url), header);
    auto resp = getHttpResponse(response);
    
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
    }
    
    return resp.first;
}

QNetworkReply::NetworkError UosFreeAccounts::getFreeAccount(int type, int llm, UosFreeAccount &freeAccount, int &status)
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

    QJsonObject sendJson;
    sendJson["mac"] = mac;
    sendJson["type"] = type;
    sendJson["model"] = llm;

    QString url = m_serverAddress + "/getFreeAccount";
    
    uos_ai::HttpClient httpClient;
    httpClient.setTimeOut(60000);
    QVariantHash header;
    header["Content-Type"] = "application/json";
    
    auto response = httpClient.post(QUrl(url), sendJson, header);
    auto resp = getHttpResponse(response);
    
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
        if (obj.contains("datas"))
            status = obj["datas"].toInt();
    }
    
    return resp.first;
}

QNetworkReply::NetworkError UosFreeAccounts::getDeterAccountLegal(const QString &appkey, int &available, QString &modelUrl, bool &claimAgain)
{
    QJsonObject sendJson;
    sendJson["appkey"] = appkey;

    QString url = m_serverAddress + "/deterAccountLegal";
    
    uos_ai::HttpClient httpClient;
    httpClient.setTimeOut(60000);
    QVariantHash header;
    header["Content-Type"] = "application/json";

    auto response = httpClient.post(QUrl(url), sendJson, header);
    auto resp = getHttpResponse(response);
    
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
    }
    
    return resp.first;
}

QNetworkReply::NetworkError UosFreeAccounts::claimAccountUsage(const QString &appkey, int &result, QString &msg)
{
    QJsonObject sendJson;
    sendJson["appkey"] = appkey;

    QString url = m_serverAddress + "/account-limit";
    
    uos_ai::HttpClient httpClient;
    httpClient.setTimeOut(60000);
    
    QVariantHash header;
    header["Content-Type"] = "application/json";

    auto response = httpClient.put(QUrl(url), sendJson, header);
    auto resp = getHttpResponse(response);
    
    auto &obj = resp.second;
    if (resp.first == QNetworkReply::NetworkError::NoError) {
        if (obj.contains("status"))
            result = obj["status"].toInt();
        if (obj.contains("desc"))
            msg = obj["desc"].toString();
    } else {
        qCWarning(logTAS) << "Failed to increase account limit, error:" << resp.first;
    }
    
    return resp.first;
}

QNetworkReply::NetworkError UosFreeAccounts::checkFreeModelActivity(int llm, int &result, UosFreeModelActivity &freeModelActivity)
{
    QString url = m_serverAddress + QString("/model-activity?model=%1").arg(llm);
    
    uos_ai::HttpClient httpClient;
    httpClient.setTimeOut(60000);
    QVariantHash header;
    header["Content-Type"] = "application/json";

    auto response = httpClient.get(QUrl(url), header);
    auto resp = getHttpResponse(response);
    
    auto &obj = resp.second;
    if (resp.first == QNetworkReply::NetworkError::NoError) {
        if (obj.contains("status"))
            result = obj["status"].toInt();

        if (result != 200) { // 查询失败
            return QNetworkReply::NetworkError::ServiceUnavailableError;
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
    }
    
    return resp.first;
}

QNetworkReply::NetworkError UosFreeAccounts::increaseUse(const QString &appkey, int chatAction)
{
    QJsonObject sendJson;
    sendJson["appkey"] = appkey;
    sendJson["useType"] = 0;//chatAction & ChatText2Image ? 1 : 0;

    QString url = m_serverAddress + "/increaseUse";
    
    uos_ai::HttpClient httpClient;
    httpClient.setTimeOut(60000);
    
    QVariantHash header;
    header["Content-Type"] = "application/json";

    auto response = httpClient.post(QUrl(url), sendJson, header);
    auto resp = getHttpResponse(response);
    
    auto &obj = resp.second;
    if (resp.first != QNetworkReply::NetworkError::NoError) {
        qCWarning(logTAS) << "Failed to increase use, error:" << resp.first << obj;
    }
    
    return resp.first;
}

QPair<QNetworkReply::NetworkError, QJsonObject> UosFreeAccounts::getHttpResponse(const uos_ai::HttpClient::HttpResponse &response)
{
    QNetworkReply::NetworkError netReplyError = response.error;
    QJsonObject obj = {};

    if (netReplyError == QNetworkReply::NetworkError::NoError) {
        QJsonDocument respJson = QJsonDocument::fromJson(response.data);
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
