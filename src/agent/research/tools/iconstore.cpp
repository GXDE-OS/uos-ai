#include "iconstore.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QSaveFile>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logResearch)

namespace uos_ai {

IconStore *IconStore::instance()
{
    static IconStore store;
    return &store;
}

IconStore::IconStore()
{
}

QString IconStore::storagePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/workspace/ai-writing/web-icons";
}

bool IconStore::ensureDir() const
{
    QDir dir(storagePath());
    if (!dir.exists() && !dir.mkpath(".")) {
        qCWarning(logResearch) << "IconStore: failed to create directory:" << storagePath();
        return false;
    }
    return true;
}

QString IconStore::filePath(const QString &key) const
{
    return storagePath() + "/" + key + ".png";
}

QString IconStore::saveFromData(const QString &key, const QByteArray &data)
{
    if (key.isEmpty() || data.isEmpty())
        return QString();

    QMutexLocker locker(&m_mutex);

    // Check cache first
    if (m_existsCache.value(key, false))
        return key;

    // Check file system
    QString path = filePath(key);
    if (QFileInfo::exists(path)) {
        m_existsCache[key] = true;
        return key;
    }

    if (!ensureDir())
        return QString();

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(logResearch) << "IconStore: failed to open file:" << path;
        return QString();
    }

    if (file.write(data) != data.size()) {
        qCWarning(logResearch) << "IconStore: write incomplete for:" << path;
        return QString();
    }
    if (!file.commit()) {
        qCWarning(logResearch) << "IconStore: failed to commit file:" << path;
        return QString();
    }

    m_existsCache[key] = true;
    return key;
}

QString IconStore::saveFromImage(const QString &key, const QImage &image)
{
    if (key.isEmpty() || image.isNull())
        return QString();

    {
        QMutexLocker locker(&m_mutex);
        if (m_existsCache.value(key, false))
            return key;
        if (QFileInfo::exists(filePath(key))) {
            m_existsCache[key] = true;
            return key;
        }
    }

    // Encode image to PNG outside the lock
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    return saveFromData(key, data);
}

QString IconStore::iconPath(const QString &key) const
{
    if (key.isEmpty())
        return QString();

    QString path = filePath(key);

    QMutexLocker locker(&m_mutex);
    if (m_existsCache.value(key, false))
        return path;

    if (QFileInfo::exists(path)) {
        m_existsCache[key] = true;
        return path;
    }

    return QString();
}

bool IconStore::exists(const QString &key) const
{
    if (key.isEmpty())
        return false;

    QMutexLocker locker(&m_mutex);
    if (m_existsCache.value(key, false))
        return true;

    if (QFileInfo::exists(filePath(key))) {
        m_existsCache[key] = true;
        return true;
    }

    return false;
}

QString IconStore::domainKey(const QString &url)
{
    QUrl parsed(url);
    QString host = parsed.host();
    if (host.isEmpty())
        return QString();

    // Strip "www." prefix
    if (host.startsWith("www."))
        host = host.mid(4);

    return host;
}

QString IconStore::extensionKey(const QString &filePath)
{
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();
    if (suffix.isEmpty())
        return QString();

    // Normalize: docx -> doc, xlsx -> xls, pptx -> ppt (same icon)
    if (suffix == "docx") suffix = "doc";
    else if (suffix == "xlsx") suffix = "xls";
    else if (suffix == "pptx") suffix = "ppt";

    return "ext_" + suffix;
}

} // namespace uos_ai
