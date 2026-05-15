// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileservice.h"
#include "parsers/fileparser.h"
#include "global_define.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QtConcurrent>

Q_DECLARE_LOGGING_CATEGORY(logService)

using namespace uos_ai;

FileService *FileService::instance()
{
    static FileService ins;
    return &ins;
}

const QStringList &FileService::supportedSuffixes()
{
    return FileParser::supportedSuffixes();
}

int FileService::checkSize(const QString &filePath) const
{
    const QFileInfo info(filePath);
    const double sizeMb = info.size() / 1024.0 / 1024.0;

    if (FileParser::strategyFor(filePath) == FileParser::Strategy::Ocr) {
        if (sizeMb > 15.0) {
            qCWarning(logService) << "Image exceeds 15 MB limit:" << filePath;
            return static_cast<int>(GErrorType::FileImageSizeExceeded);
        }
    } else {
        if (sizeMb > 100.0) {
            qCWarning(logService) << "File exceeds 100 MB limit:" << filePath;
            return static_cast<int>(GErrorType::FileSizeExceeded);
        }
    }

    return static_cast<int>(GErrorType::NoError);
}

void FileService::parseFile(const QString &id, const QString &filePath)
{
    // If a parse for this path is already in-flight, queue the id so it also
    // receives parseFinished when the current worker finishes.
    if (m_parsing.contains(filePath)) {
        qCDebug(logService) << "parseFile: coalescing with in-flight parse:" << filePath;
        m_parsing[filePath].append(id);
        return;
    }
    m_parsing.insert(filePath, QStringList{ id });

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [=] {
        const QStringList pendingIds = m_parsing.take(filePath);

        const QString text  = watcher->result();
        const int     error = text.isEmpty()
            ? static_cast<int>(GErrorType::FileParseNoText)
            : static_cast<int>(GErrorType::NoError);

        if (!text.isEmpty()) {
            QMutexLocker locker(&m_cacheMutex);
            m_cache.insert(filePath, text);
        }

        for (const QString &pendingId : pendingIds)
            emit parseFinished(pendingId, filePath, error);

        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([filePath] {
        return FileParser::parse(filePath);
    }));
}

QString FileService::cachedContent(const QString &filePath) const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.value(filePath);
}

void FileService::evict(const QString &filePath)
{
    QMutexLocker locker(&m_cacheMutex);
    m_cache.remove(filePath);
}

QString FileService::take(const QString &filePath)
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.take(filePath);
}

QString FileService::parseFileSync(const QString &filePath, QString *errorMsg)
{
    return FileParser::parse(filePath, errorMsg);
}

QString FileService::parseBase64ImagesSync(const QStringList &base64Images)
{
    return FileParser::parseBase64Images(base64Images);
}

FileService::FileService(QObject *parent)
    : QObject(parent)
{
}
