// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILECHANNEL_H
#define FILECHANNEL_H

#include <QIcon>
#include <QObject>
#include <QStringList>

namespace uos_ai {

/**
 * @brief Web ↔ Native channel for file operations.
 *
 * Registered as `window.fileObj` in QWebChannel.
 *
 * JS → C++ (slots):
 *   selectFile(params)          Open file dialog; emits FeIncomingFiles
 *   parseFile(id, filePath)     Async parse; emits FeParseResult
 *   handleDroppedFiles(params)  Process files already accepted by frontend
 *   handleCopiedFiles(params)   Process clipboard files already accepted by frontend
 *   handleScreenshotFile(params) Process screenshot image already accepted by frontend
 *
 * C++ → JS (signal):
 *   fileEvent(event, id, json)  Unified event (FileEvent enum)
 *
 * FeFileReady payload:
 *   { "file_path": "...", "file_size": 12345, "icon": "base64png", "default_prompt": "...", "category": 0, "error": 0 }
 *
 * FeParseResult payload:
 *   { "file_path": "...", "error": 0 }
 *
 * FeNativeDrop payload:
 *   { "paths": ["..."], "x": 123, "y": 456 }
 *
 * FeIncomingFiles payload:
 *   { "paths": ["..."], "default_prompt": "...", "category": 0, "backend_method": "handleDroppedFiles" }
 */
class FileChannel : public QObject
{
    Q_OBJECT
public:
    explicit FileChannel(QObject *parent = nullptr);

    void emitNativeDrop(const QStringList &paths, int x, int y);
    void emitIncomingFiles(const QStringList &paths,
                           const QString &defaultPrompt = QString(),
                           int category = -1,
                           const QString &backendMethod = QStringLiteral("handleDroppedFiles"));

signals:
    void fileEvent(int event, const QString &id, const QString &json);
    void screenshotRequested();         // 触发截图流程
    void toastRequested(const QString &type, const QString &message);

public slots:
    // params: { "plugin_only": bool, "multiple": bool, "category": number }
    void selectFile(const QString &params);

    QString getFileIconBase64(const QString &filePath, int width = 64, int height = 64);

    // params: { "paths": [...], "show_unsupported_toast": bool }
    QString validateIncomingPaths(const QString &params);

    void parseFile(const QString &id, const QString &filePath);

    // params: { "paths": [...], "default_prompt": "...", "category": number }
    void handleDroppedFiles(const QString &params);

    // params: { "paths": [...], "category": number }
    void handleCopiedFiles(const QString &params);

    // params: { "path": "..." }
    void handleScreenshotFile(const QString &params);

    // Call when the user removes a file card from the UI
    void removeFile(const QString &filePath);

    // Read system clipboard and process any files/images found.
    // Returns the plain text if the clipboard contains only text so the caller
    // can insert it at the cursor; returns "" when files/images are being
    // processed (FeFileReady will follow asynchronously).
    QString processClipboardData();

    //ScreenShot
    void startScreenshot();
    int isEnableScreenshot();
    void setCurrentAssistantId(const QString &assistantId);

    // 判断文件是否存在
    bool isFileExist(QString filePath);

private:
    void emitFileReady(const QString &filePath, int error, qint64 fileSize = -1,
                       const QString &icon = QString(),
                       const QString &defaultPrompt = QString(),
                       int category = -1);
    QStringList filterSupportedPaths(const QStringList &paths, bool *hasUnsupported = nullptr) const;

    int processOneFile(const QString &filePath,
                       const QString &defaultPrompt,
                       const QStringList &allowedSuffixes,
                       int category = -1);
    void showUnsupportedFileFormatToast();

    QString fileIconBase64(const QString &filePath, int width = 32, int height = 32);
    QIcon   fileIcon(const QString &filePath);

    QString m_lastImportPath;
    QString m_currentAssistantId;
};

} // namespace uos_ai

#endif // FILECHANNEL_H
