#include "ainetwork.h"
#include "aicodetranslation.h"
#include "servercodetranslation.h"

#include <QJsonDocument>

AINetWork::AINetWork(const AccountProxy &account)
    : BaseNetWork(account)
{

}

QString AINetWork::rootUrlPath() const
{
    return "https://api.openai.com/v1";
}

QPair<int, QByteArray> AINetWork::request(const QJsonObject &data, const QString &path, QHttpMultiPart *multipart)
{
    NetWorkResponse baseresult = BaseNetWork::request(rootUrlPath() + path, data, multipart);
    if (baseresult.error != AIServer::NoError && !baseresult.data.isEmpty()) {
        QJsonObject errorObj = QJsonDocument::fromJson(baseresult.data).object().value("error").toObject();
        if (errorObj.contains("message")) {
            const QString &message = errorObj.value("message").toString();
            if (message.contains("Rate limit reached")) {
                baseresult.error = AIServer::ServerRateLimitError;
            }
            baseresult.data = AiCodeTranslation::serverCodeTranlation(baseresult.error, message).toUtf8();
        } else {
            // 这里正在请求过程中遇到网络错误，存在残留数据，需要清理掉
            baseresult.data.clear();
        }
    }

    if (baseresult.error != AIServer::NoError && baseresult.data.isEmpty()) {
        baseresult.data = ServerCodeTranslation::serverCodeTranslation(baseresult.error, baseresult.errorString).toUtf8();
    }

    return qMakePair(baseresult.error, baseresult.data);
}
