#include "zhipunetwork.h"
#include "zhipucodetranslation.h"
#include "networkdefs.h"
#include "servercodetranslation.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>

ZhiPuNetWork::ZhiPuNetWork(const AccountProxy &account)
    : BaseNetWork(account)
{

}

QString ZhiPuNetWork::rootUrlPath() const
{
    return "https://open.bigmodel.cn/api/paas/v3/model-api";
}

QByteArray generateJwtToken(const QString &apiKey)
{
    const QStringList &apikeys = apiKey.split(".");
    if (apikeys.size() != 2) {
        qWarning() << "GenerateJwtToken Error = " << apiKey;
        return QByteArray();
    }

    QString id = apikeys.value(0);
    QString secret = apikeys.value(1);

    QJsonObject header;
    header["alg"] = "HS256";
    header["sign_type"] = "SIGN";

    QJsonObject payload;
    payload["api_key"] = id;

    qint64 msecs = QDateTime::currentMSecsSinceEpoch();
    payload["exp"] = msecs + 600000; // 10 minutes from now
    payload["timestamp"] = msecs;

    QByteArray headerBytes = QJsonDocument(header).toJson(QJsonDocument::Compact);
    QByteArray payloadBytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    QByteArray headerBase64 = headerBytes.toBase64();
    QByteArray payloadBase64 = payloadBytes.toBase64();

    QByteArray data = headerBase64 + "." + payloadBase64;
    QByteArray signature = QMessageAuthenticationCode::hash(data, secret.toUtf8(), QCryptographicHash::Sha256).toBase64();

    return data + "." + signature;
}

QPair<int, QByteArray> ZhiPuNetWork::request(const QJsonObject &data, const QString &path, QHttpMultiPart *multipart)
{
    QString token = generateJwtToken(m_accountProxy.apiKey);
    NetWorkResponse baseresult = BaseNetWork::request(rootUrlPath() + path, data, multipart, token);
    QJsonObject resultObject = QJsonDocument::fromJson(baseresult.data).object();
    if (resultObject.contains("success") && !resultObject.value("success").toBool()) {
        int code = resultObject.value("code").toInt();
        if (code == 1302 || code == 1303 || code == 1305) {
            baseresult.error = AIServer::ServerRateLimitError;
        } else {
            baseresult.error = AIServer::ContentAccessDenied;
        }
        baseresult.data = ZhiPuCodeTranslation::serverCodeTranlation(code, resultObject.value("msg").toString()).toUtf8();
    } else if (baseresult.error != AIServer::NoError) {
        // 这里正在请求过程中遇到网络错误，存在残留数据，需要清理掉
        baseresult.data.clear();
    }

    if (baseresult.error != AIServer::NoError && baseresult.data.isEmpty()) {
        baseresult.data = ServerCodeTranslation::serverCodeTranslation(baseresult.error, baseresult.errorString).toUtf8();
    }

    return qMakePair(baseresult.error, baseresult.data);
}
