#include "wxqfnetwork.h"
#include "wxqfcodetranslation.h"
#include "httpaccessmanager.h"
#include "httpeventloop.h"
#include "networkdefs.h"
#include "servercodetranslation.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

WXQFNetWork::WXQFNetWork(const AccountProxy &account)
    : BaseNetWork(account)
{

}

QPair<int, QString> WXQFNetWork::generateAccessToken(const QString &clientId, const QString &clientSecret) const
{
    QSharedPointer<HttpAccessmanager> manager = getHttpNetworkAccessManager(QString());
    QUrl url("https://aip.baidubce.com/oauth/2.0/token");
    QUrlQuery query;
    query.addQueryItem("grant_type", "client_credentials");
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("client_secret", clientSecret);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "text/event-stream");

    QNetworkReply *tokenReply = manager->post(request, "");
    HttpEventLoop loop(tokenReply, "generateAccessToken");
    loop.setHttpOutTime(15000);
    connect(this, &WXQFNetWork::requestAborted, &loop, &HttpEventLoop::abortReply);
    loop.exec();

    if (loop.getNetWorkError() != QNetworkReply::NoError) {
        int netReplyError;
        bool isAuthError = manager->isAuthenticationRequiredError();
        if (isAuthError)
            netReplyError = QNetworkReply::NetworkError::AuthenticationRequiredError;
        else if (loop.getHttpStatusCode() == 429)
            netReplyError = QNetworkReply::NetworkError::InternalServerError;
        else
            netReplyError = loop.getNetWorkError();

        return qMakePair(netReplyError, loop.getHttpResult().isEmpty() ? loop.getNetWorkErrorString() : loop.getHttpResult());
    } else {
        QJsonObject jsonObject = QJsonDocument::fromJson(loop.getHttpResult()).object();
        QString accessToken = jsonObject.value("access_token").toString();
        return qMakePair(loop.getNetWorkError(), accessToken);
    }
}

QPair<int, QByteArray> WXQFNetWork::request(const QJsonObject &data, const QString &urlPath, QHttpMultiPart *multipart)
{
    static QString accessToken;
    static QString accountId;

    QString id = m_accountProxy.appId + m_accountProxy.apiKey + m_accountProxy.apiSecret;
    if (accessToken.isEmpty() || accountId != id) {
        const QPair<int, QString> &resultTokens = generateAccessToken(m_accountProxy.apiKey, m_accountProxy.apiSecret);
        if (resultTokens.first != QNetworkReply::NoError) {
            QByteArray responseData = ServerCodeTranslation::serverCodeTranslation(resultTokens.first, resultTokens.second).toUtf8();
            AIServer::ErrorType serverErrorCode = AIServer::networkReplyErrorToAiServerError(static_cast<QNetworkReply::NetworkError>(resultTokens.first));
            return qMakePair(serverErrorCode, responseData);
        }
        accountId = id;
        accessToken = resultTokens.second;
    }

    QUrl url(urlPath);
    QUrlQuery query;
    query.addQueryItem("access_token", accessToken);
    url.setQuery(query);

    NetWorkResponse baseresult = BaseNetWork::request(url, data, multipart);
    QJsonObject resultObject = QJsonDocument::fromJson(baseresult.data).object();
    if (resultObject.contains("error_code") && resultObject.value("error_code").toInt() > 0) {
        int code = resultObject.value("error_code").toInt();
        if (code == 4 || code == 18) {
            baseresult.error = AIServer::ServerRateLimitError;
        } else {
            baseresult.error = AIServer::ContentAccessDenied;
        }
        baseresult.data = WXQFCodeTranslation::serverCodeTranlation(code, resultObject.value("error_msg").toString()).toUtf8();
    } else if (baseresult.error != AIServer::NoError) {
        // 这里正在请求过程中遇到网络错误，存在残留数据，需要清理掉
        baseresult.data.clear();
    }

    if (baseresult.error != AIServer::NoError && baseresult.data.isEmpty()) {
        baseresult.data = ServerCodeTranslation::serverCodeTranslation(baseresult.error, baseresult.errorString).toUtf8();
    }

    return qMakePair(baseresult.error, baseresult.data);
}
