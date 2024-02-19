#include "localtext2image.h"
#include "dbuslocaltexttoimagerequest.h"
#include "networkdefs.h"

LocalText2Image::LocalText2Image(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{

}

QList<QByteArray> LocalText2Image::text2Image(const QString &prompt, int number)
{
    Q_UNUSED(number)

    QList<QByteArray> imageData;
    DbusLocalTextToImageRequest dbus;
    connect(this, &LocalText2Image::aborted, &dbus, &DbusLocalTextToImageRequest::requestAborted);

    QByteArray image = dbus.textToImage(prompt, "", true);

    if (image.isEmpty()) {
        setLastError(AIServer::ServiceUnavailableError);
        setLastErrorString("本地模型生成图片失败");
    } else {
        setLastError(AIServer::NoError);
    }

    return imageData << image;
}

QJsonObject LocalText2Image::predict(const QString &content, const QJsonArray &functions, const QString &systemRole, qreal temperature)
{
    Q_UNUSED(content)
    Q_UNUSED(functions)
    Q_UNUSED(systemRole)
    Q_UNUSED(temperature)

    QJsonObject response;
    return response;
}

QPair<int, QString> LocalText2Image::verify()
{
    QPair<int, QString> errorpair;

    return errorpair;
}
