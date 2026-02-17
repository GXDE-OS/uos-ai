#include "localtext2image.h"
#include "dbuslocaltexttoimagerequest.h"
#include "networkdefs.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

LocalText2Image::LocalText2Image(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{

}

QList<QByteArray> LocalText2Image::text2Image(const QString &prompt, int number)
{
    Q_UNUSED(number)
    qCInfo(logLLM) << "LocalText2Image Starting text-to-image generation with prompt:" << prompt.size() << "characters";

    QList<QByteArray> imageData;
    DbusLocalTextToImageRequest dbus;
    connect(this, &LocalText2Image::aborted, &dbus, &DbusLocalTextToImageRequest::requestAborted);

    QByteArray image = dbus.textToImage(prompt, "", true);

    if (image.isEmpty()) {
        qCWarning(logLLM) << "LocalText2Image Failed to generate image from local model";
        setLastError(AIServer::ServiceUnavailableError);
        setLastErrorString("本地模型生成图片失败");
    } else {
        qCDebug(logLLM) << "LocalText2Image Successfully generated image from local model";
        setLastError(AIServer::NoError);
    }

    return imageData << image;
}

QJsonObject LocalText2Image::predict(const QString &content, const QJsonArray &functions)
{
    Q_UNUSED(content)
    Q_UNUSED(functions)

    QJsonObject response;
    return response;
}

QPair<int, QString> LocalText2Image::verify()
{
    QPair<int, QString> errorpair;

    return errorpair;
}
