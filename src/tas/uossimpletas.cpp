#include "uossimpletas.h"
#include "httpaccessmanager.h"
#include "httpeventloop.h"
#include "networkdefs.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logTAS)

UosSimpleTas::UosSimpleTas()
    : TAS()
{
}

QString UosSimpleTas::hostUrl() const
{
    //return "https://appstore.uniontech.com/store-dist-comment/comment/audit";
    //return "http://127.0.0.1:3000/audit";
    return "http://10.4.17.162:9600/api/comment/audit";
}

TextAuditResult UosSimpleTas::doTextAuditing(const QByteArray &data)
{
    QJsonDocument document;
    QJsonObject auditTextJson;
    auditTextJson["content"] = data.data();
    document.setObject(auditTextJson);
    const QByteArray &sendData = document.toJson(QJsonDocument::Compact);

    QSharedPointer<HttpAccessmanager> httpAccessManager
        = QSharedPointer<HttpAccessmanager>(new HttpAccessmanager(""));

    QNetworkRequest req = httpAccessManager->baseNetWorkRequest(QUrl(hostUrl()), false);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    qCInfo(logTAS) << "Sending audit request, data length:" << data.length();
    QNetworkReply *reply = httpAccessManager->post(req, sendData);

    HttpEventLoop loop(reply, "UosSimpleTas::doTextAuditing");
    loop.setHttpOutTime(15000);
    loop.exec();

    TextAuditResult txtResult{};
    QNetworkReply::NetworkError netReplyError;
    bool isAuthError = httpAccessManager->isAuthenticationRequiredError();
    if (isAuthError)
        netReplyError = QNetworkReply::NetworkError::AuthenticationRequiredError;
    else if (loop.getHttpStatusCode() == 429)
        netReplyError = QNetworkReply::NetworkError::InternalServerError;
    else
        netReplyError = loop.getNetWorkError();

    if (netReplyError == QNetworkReply::NetworkError::NoError) {
        qCDebug(logTAS) << "Audit response received, length:" << loop.getHttpResult().length();
        QJsonDocument respJson = QJsonDocument::fromJson(loop.getHttpResult());
        if (respJson.isObject()) {
            auto resultObj = respJson.object();
            if (resultObj.contains("datas")) {
                auto datasObj = resultObj["datas"].toObject();
                if (datasObj.contains("suggestion")) {
                    if (datasObj["suggestion"].toString() == "pass") {
                        txtResult.code = None;
                        qCInfo(logTAS) << "Audit passed";
                    } else {
                        txtResult.code = Contraband;
                        qCWarning(logTAS) << "Audit failed, contraband detected";
                    }
                } else {
                    txtResult.code = NetError;
                    qCWarning(logTAS) << "Audit response missing suggestion field";
                }
            }
        }
    } else {
        txtResult.code = NetError;
        qCCritical(logTAS) << "Audit request failed, network error:" << netReplyError;
    }

    return txtResult;
}

