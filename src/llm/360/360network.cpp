#include "360network.h"
#include "360codetranslation.h"
#include "servercodetranslation.h"

#include <QJsonDocument>

NetWork360::NetWork360(const AccountProxy &account)
    : BaseNetWork(account)
{

}

QString NetWork360::rootUrlPath() const
{
    return "https://api.360.cn/v1";
}

QPair<int, QByteArray> NetWork360::request(const QJsonObject &data, const QString &path, QHttpMultiPart *multipart)
{
    NetWorkResponse baseresult = BaseNetWork::request(rootUrlPath() + path, data, multipart);
    if (baseresult.error != AIServer::NoError && !baseresult.data.isEmpty()) {
        QJsonObject dataObj = QJsonDocument::fromJson(baseresult.data).object();
        if (dataObj.contains("error")) {
            QJsonObject errorObj = dataObj.value("error").toObject();
            int code = errorObj.value("code").toVariant().toInt();
            QString messgae = errorObj.value("message").toString();
            if (code == 1005) {
                baseresult.error = AIServer::ServerRateLimitError;
            } else if (code == 1002) {
                baseresult.error = AIServer::AuthenticationRequiredError;
            } else {
                baseresult.error = AIServer::ContentAccessDenied;
            }

            QString errorMessage = ServerCodeTranslation::serverCodeTranslation(baseresult.error, QString());
            if (errorMessage.isEmpty())
                baseresult.data = CodeTranslation360::serverCodeTranlation(code, messgae).toUtf8();
            else
                baseresult.data = errorMessage.toUtf8();
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
