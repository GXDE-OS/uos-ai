#include "dbuslocaltexttoimagerequest.h"

#include <QDebug>
#include <QFile>
#include <QBuffer>

#define LOCAL_TEXT2IMAGE_SERVICE      "com.deepin.texttoimage"
#define LOCAL_TEXT2IMAGE_PATH         "/com/deepin/texttoimage"
#define LOCAL_TEXT2IMAGE_INTERFACE    "com.deepin.texttoimage"

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
    QDBusMessage msg = call(QDBus::Block, "textToImage", prompt, "dog", isChinese);
    if (!msg.errorMessage().isEmpty()) {
        qInfo() << "DbusLocalTextToImageRequest::textToImage=" << prompt << ",error=" << msg;
        return m_image;
    }

    m_event.exec();
    return m_image;
}

void DbusLocalTextToImageRequest::finished(const QString &path)
{
    qInfo() << "DbusLocalTextToImageRequest::finished=" << path;
    QFile file(path);
    if (!file.exists()) {
        qInfo() << "文件不存在" << path;
        m_event.quit();
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qInfo() << "无法打开文件" << path;
        m_event.quit();
        return;
    }

    m_image = file.readAll();
    file.close();

    m_event.quit();
}

void DbusLocalTextToImageRequest::requestAborted()
{
    m_event.quit();
}
