#include "cozeagentchat.h"
#include "modelinfo.h"
#include "global_key_define.h"
#include "network/httpclient.h"
#include "network/httpcodetranslation.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QUuid>
#include <QMutex>
#include <QNetworkInterface>
#include <QCryptographicHash>

#include <jwt.h>

Q_DECLARE_LOGGING_CATEGORY(logModel)

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

namespace  {

class AutoUnlock
{
public:
    AutoUnlock(QMutex *m) : mtx(m){}
    ~AutoUnlock(){if (mtx)mtx->unlock();}
protected:
    QMutex *mtx = nullptr;
};

static QPair<QString, qint64> oauthToken = {"", 0};
static QString userID = "";
static QMutex tokenMutex;

}

CozeAgentChat::CozeAgentChat(QObject *parent) : AbstractChatModel(parent)
{
    if (userID.isEmpty()) {
        userID = getUniqueIdentifier();
    }
}

CozeAgentChat::~CozeAgentChat()
{

}

QString CozeAgentChat::generateJWTToken()
{
    std::string jti = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    const std::string iss = "1294484808703";
    const std::string kid = "HBZ_qGDBH159ri2hIt-q0K5Xqq33P-_gcVk1P-84Dt0";
    const std::string aud = "api.coze.cn";
    const qint64 iat = QDateTime::currentSecsSinceEpoch();
    const qint64 exp = iat + 15 * 60;

    jwt_t *jwt = nullptr;
    jwt_new(&jwt);
    if (!jwt)
        return "";

    jwt_add_header(jwt, "alg", "RS256");
    jwt_add_header(jwt, "typ", "JWT");
    jwt_add_header(jwt, "kid", kid.c_str());

    jwt_add_grant(jwt, "iss", iss.c_str());
    jwt_add_grant(jwt, "aud", aud.c_str());
    jwt_add_grant_int(jwt, "iat", iat);
    jwt_add_grant_int(jwt, "exp", exp);
    jwt_add_grant(jwt, "jti", jti.c_str());

    jwt_set_alg(jwt, JWT_ALG_RS256, (const unsigned char *)rsa_priv_key.c_str(), rsa_priv_key.size());

    char *jwtStr = jwt_encode_str(jwt);
    if (!jwtStr) {
        jwt_free(jwt);
        return "";
    }

    QString jwtKey = jwtStr;
    free(jwtStr);
    jwt_free(jwt);

    return jwtKey;
}

QString CozeAgentChat::getUniqueIdentifier()
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

QPair<int, QString> CozeAgentChat::ensureToken(QString &token)
{
    if (!tokenMutex.tryLock(100)) {
        qCWarning(logModel) << "Coze Failed to acquire token mutex within timeout";
        return {QNetworkReply::NetworkError::InternalServerError, ""};
    }

    AutoUnlock locker(&tokenMutex);

    auto cur = QDateTime::currentSecsSinceEpoch();
    if (!oauthToken.first.isEmpty() && oauthToken.second > (cur + 10)) {
        token = oauthToken.first;
        qCDebug(logModel) << "Coze Using cached token, expires in" << (oauthToken.second - cur) << "seconds";
        return {0, ""};
    }

    QString jwtToken = generateJWTToken();
    if (jwtToken.isEmpty()) {
        return {QNetworkReply::NetworkError::AuthenticationRequiredError, ""};
    }

    HttpClient client;
    QUrl url("https://api.coze.cn/api/permission/oauth2/token");

    QVariantHash header;
    header.insert("Content-Type", "application/json");
    header.insert("Authorization", QString("Bearer %0").arg(jwtToken));

    QJsonObject postData;
    postData["duration_seconds"] = 900;
    postData["grant_type"] = "urn:ietf:params:oauth:grant-type:jwt-bearer";

    connect(this, &CozeAgentChat::requestCancel, &client, &HttpClient::doAbort);

    HttpClient::HttpResponse response = client.post(url, postData, header);

    if (response.error != QNetworkReply::NoError) {
        return {response.error, response.errorString};
    } else {
        QJsonObject jsonObject = QJsonDocument::fromJson(response.data).object();
        oauthToken.first = jsonObject.value("access_token").toString();
        oauthToken.second = jsonObject.value("expires_in").toDouble();
        token = oauthToken.first;
        return {0, QString()};
    }
}

QJsonArray CozeAgentChat::buildMessages(const QList<ModelMessage> &messages)
{
    QJsonArray ret;
    for (const ModelMessage &msg : messages) {
        QJsonObject msgObj;
        msgObj["role"] = msg.role;
        msgObj["content_type"] = "text";

        QString content;
        for (const MetaMessage &metaMsg : msg.content) {
            if (metaMsg.type == CntText) {
                content += metaMsg.data.toString();
            }
        }
        msgObj["content"] = content;
        ret.append(msgObj);
    }
    return ret;
}

QString CozeAgentChat::parseContentString(const QByteArray &content)
{
    m_deltaContent += content;
    if (!content.trimmed().endsWith("}")) {
        return "";
    }

    QString tmpText;
    {
        QRegularExpression regex(R"(event:\s*conversation\.message\.delta\ndata:\s*\{(.*)\})");
        QRegularExpressionMatchIterator iter = regex.globalMatch(m_deltaContent);
        while (iter.hasNext()) {
            QRegularExpressionMatch match = iter.next();
            QString matchString = match.captured(0);

            int startIndex = matchString.indexOf('{');
            int endIndex = matchString.lastIndexOf('}');
            if (startIndex >= 0 && endIndex > startIndex) {
                QString content = matchString.mid(startIndex, endIndex - startIndex + 1);
                QJsonObject j = QJsonDocument::fromJson(content.toUtf8()).object();
                if (j.contains("role") && j.contains("content")) {
                    QString role = j.value("role").toString();
                    QString text = j.value("content").toString();
                    QString type = j.value("type").toString();
                    if (role == "assistant" && type == "answer") {
                        tmpText.append(text);
                        m_answerContent.append(text);
                    }
                }
            }
        }
    }

    if (m_answerContent.isEmpty()) {
        QRegularExpression regex(R"(event:\s*conversation\.chat\.failed\ndata:\s*\{(.*)\})");
        QRegularExpressionMatchIterator iter = regex.globalMatch(m_deltaContent);
        while (iter.hasNext()) {
            QRegularExpressionMatch match = iter.next();
            QString matchString = match.captured(0);

            int startIndex = matchString.indexOf('{');
            int endIndex = matchString.lastIndexOf('}');
            if (startIndex >= 0 && endIndex > startIndex) {
                QString scontent = matchString.mid(startIndex, endIndex - startIndex + 1);
                QJsonObject j = QJsonDocument::fromJson(scontent.toUtf8()).object();
                if (j.contains("last_error")) {
                    j = j.value("last_error").toObject();
                    if (j.contains("code") && j.contains("msg")) {
                        m_chatFailed.first = j.value("code").toInt();
                        m_chatFailed.second = j.value("msg").toString();
                    }
                }
            }
        }
    }

    m_deltaContent.clear();
    return tmpText;
}

QVariantHash CozeAgentChat::chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams)
{
    m_error.clear();
    m_answerContent.clear();
    m_deltaContent.clear();
    m_chatFailed = {0, ""};

    QString token;
    auto ret = ensureToken(token);
    if (token.isEmpty()) {
        m_error[STR_KEY_ERROR] = GErrorType::HttpError;
        m_error[STR_KEY_HTTP_ERROR] = ret.first;
        m_error[STR_KEY_ERROR_MESSAGE] = ret.second;
        return QVariantHash();
    }

    QJsonObject dataObject;
    dataObject.insert("bot_id", botId());
    dataObject.insert("user_id", userID);
    dataObject.insert("auto_save_history", false);
    dataObject.insert("stream", true);
    dataObject.insert("additional_messages", buildMessages(messages));

    HttpClient client;
    client.setTimeOut(m_parameters.value(STR_KEY_TIMEOUT, 10 * 60 * 1000).toInt());

    connect(this, &CozeAgentChat::requestCancel, &client, &HttpClient::doAbort);
    connect(&client, &HttpClient::readyRead, this, &CozeAgentChat::chunkReceived);

    QString url = apiHost();
    QVariantHash header;
    header.insert("Authorization", QString("Bearer %0").arg(token));

    HttpClient::HttpResponse response = client.request(url, dataObject, header);

    QVariantHash result;
    result[STR_KEY_CONTENT] = m_answerContent;

    if (response.error != QNetworkReply::NoError || m_chatFailed.first != 0) {
        m_error[STR_KEY_ERROR] = GErrorType::HttpError;
        m_error[STR_KEY_HTTP_ERROR] = m_chatFailed.first != 0 ? m_chatFailed.first : response.error;
        m_error[STR_KEY_ERROR_MESSAGE] = m_chatFailed.second.isEmpty() ? response.errorString : m_chatFailed.second;
    }

    return result;
}

void CozeAgentChat::setApiHost(const QString &host)
{
    m_host = host;
}

QString CozeAgentChat::apiHost() const
{
    return m_host;
}

void CozeAgentChat::setBotId(const QString &botId)
{
    m_botId = botId;
}

QString CozeAgentChat::botId() const
{
    return m_botId;
}

void CozeAgentChat::cancel()
{
    emit requestCancel();
}

void CozeAgentChat::chunkReceived(const QByteArray &chunk)
{
    QString cur = parseContentString(chunk);
    if (!cur.isEmpty()) {
        QVariantHash content;
        content[STR_KEY_CONTENT] = cur;
        MetaMessageList msgs = MetaMessage::fromHash(content);
        emit messageReceived(msgs);
    }
}
