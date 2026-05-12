// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILESERVICE_H
#define FILESERVICE_H

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QStringList>

namespace uos_ai {

/**
 * @brief Service layer for async file parsing.
 *
 * Sits between FileChannel (UI/protocol) and FileParser (pure extraction).
 * Manages QFutureWatcher lifetime and exposes validation helpers so that
 * FileChannel does not need to know about FileParser internals.
 *
 * Usage:
 *   FileService::instance()->parseFile(id, filePath);
 *   connect(FileService::instance(), &FileService::parseFinished, ...);
 */
class FileService : public QObject
{
    Q_OBJECT
public:
    static FileService *instance();

    /**
     * @brief Supported file suffixes (delegates to FileParser::supportedSuffixes).
     */
    static const QStringList &supportedSuffixes();

    /**
     * @brief Check whether a file is within the allowed size limit.
     * @return GErrorType::NoError, FileImageSizeExceeded, or FileSizeExceeded.
     */
    int checkSize(const QString &filePath) const;

    // ── Async ────────────────────────────────────────────────────────────────

    /**
     * @brief Start async text extraction for a file on the thread pool.
     *        On success, caches the result; emits parseFinished when done.
     *        Must be called from the main thread.
     */
    void parseFile(const QString &id, const QString &filePath);

    /**
     * @brief Return the cached extracted text for a file path.
     *        Thread-safe.
     *        Returns an empty string if the file has not been parsed yet.
     */
    QString cachedContent(const QString &filePath) const;

    /**
     * @brief Remove a cached entry (call when the user removes a file card).
     *        Thread-safe.
     */
    void evict(const QString &filePath);

    /**
     * @brief Retrieve and remove a cached entry in a single operation.
     *        Returns an empty string if no cached content exists.
     *        Thread-safe.
     */
    QString take(const QString &filePath);

    // ── Sync ─────────────────────────────────────────────────────────────────

    /**
     * @brief Blocking text extraction. Safe to call from any thread.
     * @param errorMsg  Human-readable error message on failure (may be nullptr).
     * @return Extracted text, or empty string on failure.
     */
    static QString parseFileSync(const QString &filePath,
                                 QString *errorMsg = nullptr);

    /**
     * @brief Blocking OCR for base64-encoded images. Safe to call from any thread.
     */
    static QString parseBase64ImagesSync(const QStringList &base64Images);

signals:
    // content is intentionally omitted — callers use cachedContent() instead
    void parseFinished(const QString &id, const QString &filePath, int error);

private:
    explicit FileService(QObject *parent = nullptr);

    QHash<QString, QString>      m_cache;    // filePath → extracted text (guarded by m_cacheMutex)
    QHash<QString, QStringList>  m_parsing;  // filePath → pending request IDs (main-thread only)
    mutable QMutex               m_cacheMutex;
};

} // namespace uos_ai

#endif // FILESERVICE_H
