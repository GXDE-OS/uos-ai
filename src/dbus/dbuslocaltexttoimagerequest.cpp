#include "dbuslocaltexttoimagerequest.h"

#include <QFile>
#include <QBuffer>
#include <QLoggingCategory>

#define LOCAL_TEXT2IMAGE_SERVICE      "com.deepin.texttoimage"
#define LOCAL_TEXT2IMAGE_PATH         "/com/deepin/texttoimage"
#define LOCAL_TEXT2IMAGE_INTERFACE    "com.deepin.texttoimage"

Q_DECLARE_LOGGING_CATEGORY(logDBus)

DbusLocalTextToImageRequest::DbusLocalTextToImageRequest(QObject *parent)
    : QDBusInterface(LOCAL_TEXT2IMAGE_SERVICE, LOCAL_TEXT2IMAGE_PATH, LOCAL_TEXT2IMAGE_INTERFACE, QDBusConnection::systemBus(), parent)
{
    QDBusConnection::systemBus().connect(LOCAL_TEXT2IMAGE_SERVICE,
                                         LOCAL_TEXT2IMAGE_PATH,
                                         LOCAL_TEXT2IMAGE_INTERFACE,
                                         "finished",
                                         this,
                                         SLOT(finished(const QString &)));
}

QByteArray DbusLocalTextToImageRequest::textToImage(const QString &prompt, const QString &negativePrompt, bool isChinese)
{
    qCDebug(logDBus) << "Converting text to image - prompt:" << prompt << "isChinese:" << isChinese;
    
    QDBusMessage msg = call(QDBus::Block, "textToImage", prompt, "dog", isChinese);
    if (!msg.errorMessage().isEmpty()) {
        qCWarning(logDBus) << "Text to image conversion failed:" << msg.errorMessage();
        return m_image;
    }

    m_event.exec();
    qCDebug(logDBus) << "Text to image conversion completed";
    return m_image;
}

void DbusLocalTextToImageRequest::finished(const QString &path)
{
    qCDebug(logDBus) << "Text to image conversion finished, path:" << path;
    
    QFile file(path);
    if (!file.exists()) {
        qCWarning(logDBus) << "Image file does not exist:" << path;
        m_event.quit();
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(logDBus) << "Failed to open image file:" << path;
        m_event.quit();
        return;
    }

    m_image = file.readAll();
    file.close();
    qCDebug(logDBus) << "Image file read successfully, size:" << m_image.size();

    m_event.quit();
}

void DbusLocalTextToImageRequest::requestAborted()
{
    qCDebug(logDBus) << "Text to image request aborted";
    m_event.quit();
}
