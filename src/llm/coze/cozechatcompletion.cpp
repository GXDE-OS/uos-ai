// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cozechatcompletion.h"
#include "httpaccessmanager.h"
#include "httpeventloop.h"
#include "networkdefs.h"
#include "servercodetranslation.h"

#include <QUuid>
#include <QMutex>
#include <QNetworkInterface>
#include <QLoggingCategory>

#include <jwt.h>

Q_DECLARE_LOGGING_CATEGORY(logLLM)
namespace  {

class AutoUnlock
{
public:
    AutoUnlock(QMutex *m) : mtx(m){}
    ~AutoUnlock(){if (mtx)mtx->unlock();}
protected:
    QMutex *mtx = nullptr;
};

}

using namespace uos_ai;

static const std::string rsa_priv_key = R"(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCyP5tJrJQ9GwXE
abMpCCZgRN9h4C+4Tl5VcApXC/yl3u93BtnXqH5kscJV/kwExvhbAqrb68MyV99S
JnzcTgUCiNj+coZWeiSzsP5n1Yy064k4l2nwY5MCGKFHrG9cmmO+NMH07sLcuCI5
EqW8N3uLVnt6irCx2ABMMvUOZUSExKErJJbXGwtD8pXAqmvVT89ew/JujfZASDe4
/SC/zAMtutNWv50Qyj7UwOdJE8GnN2TB3kxkOy33gefFMHainXnsgxz44SZx6Rtr
qv9ijnUzhU97vyXYcoYCDohwG+LeLy4kEpf313Y+AMPq1unTgGKB3M9XeFpRRD7w
G5AknOVNAgMBAAECggEADJSbr6/8CHILTqkNKZS3LK0/vOc3RwFU/B+7wxgH/WcG
LLbxkhyEyzOWnT4k/GNUrQu7pmj17mwwSVqJDn19MggZlJ/dpmXZiEsyMX7rfutu
1G0vX3WPryKw41+ivnzxFEAh/lkJrM1cMP4Fb4rU7+krLuNyBCNUJgykqh7wnpQp
zJOhMpQby2JzW0iBOoUxTIlQ+99i2cQ2NdMv5ilEqh1bJWh4oDwzUNEN0jxGNFBe
nLJrj5vollpjWnANM49qS9RV+Ww69t50r5IUj4POGxClRJ52xs6KA/r0TezGYsNm
p0oc1XgxgSwUXGU0d6HA/OcANwbrKoccHtZf0wVm6wKBgQDycjY+kI1tqYf43WP7
EnAEUdvHPzs7q5BR+kY9Nfnwy0LWdxF2TLdYVOgtutYoUndeUtVKEaGXLiay1sfO
B/ZvNaCJ307nm5TEHyPZ0Sa6ICgOhd/D9jI7DPXqT64rZ/L/qNByDswD1O7m7aJq
0JQtbH8oQqNOitRUN6jYnwuikwKBgQC8Np/Sfou2l413knNvg7q/26SlbqxSFJci
X7EtBIoo/+zVyQmblqwtM8EAb+DfAYjEUev3VcGe2jzr4pH3Wrje+l8VunLtHUJL
np437/hehIgZraJQhJ3ybnFK+hj3N7Tk+OfDJFiES19RNzslKtCAGHAowQgspzwa
/A2CSQjknwKBgH8reViH6idDbZcUSYVAvpEFfwqcGC3MZ/8YEA+7aEbt1zk3a2lq
BCHJJ6AVCJBN8V8Ag/A+H0x1YMcd8eAYPSNINdUb1P+XlAjrbJqsImwuwdQcVKst
UFu5CKTG9sy90bBYlD2/lTPNVSK8Mx1kGtVYu7oaeh6Qo58pMt+tTSURAoGADMEC
2Ye7vfbNHlItQlxB1mhy4sb6JGHC0BB9+Bf7qMWW7fG+le2C41lDdd0e9t2qv+rx
O4RzJ2mQgUeMLp/WMT30HYvR5+F4sZkhDZdUVEtU0bCjdCNYdKopQTRrZq0+s9lu
ExouaWCWJ/G/L0iUcmm1h/10qVn3x7hWzgJW+O8CgYEA3uqmJtyb2oQIDu1QAPcp
Lg1EWsZHUNpXkTrXwuoXwwp95DDYBBNtk2KQX90Cu6E7z9wgiydQPObk6ep3KLAp
xUxcsxxjVtsrWXWBhunZtcGtAHYy8LBz/J2GjFTGJ5Ne+V6xYX7TSe64BRxKjt1w
h4tNI1eg6HJTrtgtlU4Q6go=
-----END PRIVATE KEY-----)";

QPair<QString, qint64> CozeChatCompletion::oauthToken = {"", 0};
QString CozeChatCompletion::userID = "";

CozeChatCompletion::CozeChatCompletion(const LLMServerProxy &account)
    : BaseNetWork(account.account)
    , severPorxy(account)
{
    setTimeOut(5 * 60 * 1000);

    if (userID.isEmpty()) {
        userID = getUniqueIdentifier();
        qCDebug(logLLM) << "Coze Generated new user ID:" << userID;
    }
}

QPair<int, QString> CozeChatCompletion::ensureToken(QString &token)
{
    static QMutex mtx;
    if (!mtx.tryLock(100)) {
        qCWarning(logLLM) << "Coze Failed to acquire token mutex within timeout";
        return {QNetworkReply::NetworkError::InternalServerError, ""};
    }

    AutoUnlock lk(&mtx);

    auto cur = QDateTime::currentSecsSinceEpoch();
    if (!oauthToken.first.isEmpty() && oauthToken.second > (cur + 10)) {
        token = oauthToken.first;
        qCDebug(logLLM) << "Coze Using cached token, expires in" << (oauthToken.second - cur) << "seconds";
        return {0, ""};
    }

    //see https://www.coze.cn/docs/developer_guides/oauth_jwt_channel
    QString jwtToken = generateJWTToken();
    if (jwtToken.isEmpty()) {
        qCCritical(logLLM) << "Coze Failed to generate JWT token";
        return {QNetworkReply::NetworkError::AuthenticationRequiredError, ""};
    }

    qCInfo(logLLM) << "Coze Requesting new OAuth token from Coze API";
    QSharedPointer<HttpAccessmanager> manager = getHttpNetworkAccessManager(QString());
    QUrl url("https://api.coze.cn/api/permission/oauth2/token");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Authorization", (QString("Bearer %0").arg(jwtToken)).toUtf8());

    QJsonObject postData;
    postData["duration_seconds"] = 900; //15分钟有效期
    postData["grant_type"] = "urn:ietf:params:oauth:grant-type:jwt-bearer";

    QJsonDocument jsonDoc(postData);
    QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);

    QNetworkReply *tokenReply = manager->post(request, jsonData);
    HttpEventLoop loop(tokenReply, "generateAccessToken");
    loop.setHttpOutTime(15000);
    connect(this, &CozeChatCompletion::requestAborted, &loop, &HttpEventLoop::abortReply);
    loop.exec();

    if (loop.getNetWorkError() != QNetworkReply::NoError) {
        qCWarning(logLLM) << "Coze Token request failed with error:" << loop.getNetWorkErrorString();
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
        oauthToken.first = jsonObject.value("access_token").toString();
        oauthToken.second = jsonObject.value("expires_in").toDouble();
        token = oauthToken.first;
        return qMakePair(0, QString());
    }
}

QPair<int, QString> CozeChatCompletion::create(const QJsonArray &msg)
{
    QString token;
    auto ret = ensureToken(token);
    if (token.isEmpty()) {
        qCWarning(logLLM) << "Coze Failed to obtain token for chat completion:" << ret.second;
        return ret;
    }

    qCInfo(logLLM) << "Coze Creating chat completion with bot:" << botId();
    QJsonObject dataObject;
    dataObject.insert("bot_id", botId());
    dataObject.insert("user_id", userID);
    dataObject.insert("auto_save_history", false);
    dataObject.insert("stream", true);
    dataObject.insert("additional_messages", msg);

    NetWorkResponse baseresult = BaseNetWork::request(apiUrl(), dataObject, nullptr, token);
    QJsonObject resultObject = QJsonDocument::fromJson(baseresult.data).object();
    int errCode = resultObject.value("code").toInt();
    if (errCode != 0) {
        qCWarning(logLLM) << "Coze Chat completion failed with code:" << errCode << "message:" << resultObject.value("msg").toString();
        return qMakePair(errCode, resultObject.value("msg").toString());
    }

    return qMakePair(AIServer::NoError, QString());
}

QString CozeChatCompletion::generateJWTToken()
{
    //see https://www.coze.cn/docs/developer_guides/oauth_jwt_channel

    std::string jti = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString(); // 随机字符串，防止重放攻击
    const std::string iss = "1294484808703"; //OAuth 应用的 ID
    const std::string kid = "HBZ_qGDBH159ri2hIt-q0K5Xqq33P-_gcVk1P-84Dt0"; //OAuth 应用的公钥指纹
    const std::string aud = "api.coze.cn";
    const qint64 iat = QDateTime::currentSecsSinceEpoch(); // JWT开始生效的时间，秒级时间戳
    const qint64 exp = iat + 15 * 60; // JWT过期时间，秒级时间戳

    jwt_t *jwt = nullptr;
    jwt_new(&jwt);
    if (!jwt)
        return "";

    // Header
    jwt_add_header(jwt, "alg", "RS256");
    jwt_add_header(jwt, "typ", "JWT");
    jwt_add_header(jwt, "kid", kid.c_str());

    //Payload
    jwt_add_grant(jwt, "iss", iss.c_str());
    jwt_add_grant(jwt, "aud", aud.c_str());
    jwt_add_grant_int(jwt, "iat", iat);
    jwt_add_grant_int(jwt, "exp", exp);
    jwt_add_grant(jwt, "jti", jti.c_str());

    jwt_set_alg(jwt, JWT_ALG_RS256, (const unsigned char *)rsa_priv_key.c_str(), rsa_priv_key.size());

    char *jwtStr = jwt_encode_str(jwt);
    if (!jwtStr) {
        qCCritical(logLLM) << "Coze Failed to encode JWT token";
        jwt_free(jwt);
        return "";
    }

    QString jwtKey = jwtStr;
    free(jwtStr);
    jwt_free(jwt);

    return jwtKey;
}

QUrl CozeChatCompletion::apiUrl() const
{
    return QUrl("https://api.coze.cn/v3/chat");
}

QString CozeChatCompletion::botId() const
{
    return severPorxy.ext.value(LLM_EXTKEY_VENDOR_PARAMS).toHash().value("botID").toString();
}

QString CozeChatCompletion::getUniqueIdentifier()
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    QList<QByteArray> macs;
    foreach(QNetworkInterface interface, interfaces) {
        QString mac = interface.hardwareAddress();
        if (!mac.isEmpty()) {
            macs << mac.toUtf8();
        }
    }
    QByteArray hash = QCryptographicHash::hash(macs.join(':'), QCryptographicHash::Sha256);
    return hash.toHex();
}
