// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filechannel.h"
#include "global_define.h"
#include "global_key_define.h"
#include "services/fileservice/fileservice.h"
#include "services/screenshot/screenshotservice.h"
#include "gui/web/webcontext.h"

#include <DFileDialog>
#include <DFileIconProvider>

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMimeData>
#include <QStandardPaths>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DWIDGET_USE_NAMESPACE

using namespace uos_ai;

FileChannel::FileChannel(QObject *parent)
    : QObject(parent)
{
    m_lastImportPath =
        QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();

    connect(FileService::instance(), &FileService::parseFinished,
            this, [this](const QString &id, const QString &filePath, int error) {
        QJsonObject json;
        json[STR_KEY_FILE_PATH] = filePath;
        json[STR_KEY_ERROR]     = error;
        emit fileEvent(FileEvent::FeParseResult, id,
                       QJsonDocument(json).toJson(QJsonDocument::Compact));
    });
}

void FileChannel::emitNativeDrop(const QStringList &paths, int x, int y)
{
    if (paths.isEmpty())
        return;

    QJsonObject json;
    json["x"] = x;
    json["y"] = y;

    QJsonArray pathsArray;
    for (const QString &path : paths) {
        if (!path.isEmpty()) {
            pathsArray.append(path);
        }
    }

    if (pathsArray.isEmpty())
        return;

    json["paths"] = pathsArray;
    emit fileEvent(FileEvent::FeNativeDrop, QString(),
                   QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void FileChannel::emitIncomingFiles(const QStringList &paths,
                                    const QString &defaultPrompt,
                                    int category,
                                    const QString &backendMethod)
{
    if (paths.isEmpty())
        return;

    QJsonArray pathsArray;
    for (const QString &path : paths) {
        if (!path.isEmpty()) {
            pathsArray.append(path);
        }
    }

    if (pathsArray.isEmpty())
        return;

    QJsonObject json;
    json["paths"] = pathsArray;
    if (!defaultPrompt.isEmpty())
        json[STR_KEY_DEFAULT_PROMPT] = defaultPrompt;
    if (category >= 0)
        json[STR_KEY_CATEGORY] = category;
    if (!backendMethod.isEmpty())
        json["backend_method"] = backendMethod;

    emit fileEvent(FileEvent::FeIncomingFiles, QString(),
                   QJsonDocument(json).toJson(QJsonDocument::Compact));
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────

void FileChannel::selectFile(const QString &params)
{
    QJsonObject root;
    if (!params.isEmpty()) {
        QJsonParseError err;
        root = QJsonDocument::fromJson(params.toUtf8(), &err).object();
        if (err.error != QJsonParseError::NoError)
            qCWarning(logAIGUI) << "selectFile: bad params JSON:" << err.errorString();
    }

    const bool pluginOnly      = root.value(STR_KEY_PLUGIN_ONLY).toBool(false);
    const bool multiple        = root.value(STR_KEY_MULTIPLE).toBool(false);
    const int category         = root.value(STR_KEY_CATEGORY).toInt(-1);
    const QStringList suffixes = pluginOnly
        ? QStringList{ "*.docx" }
        : FileService::supportedSuffixes();

    DFileDialog dlg(qApp->activeWindow());
    dlg.setDirectory(m_lastImportPath);
    dlg.setViewMode(DFileDialog::Detail);
    dlg.setFileMode(multiple ? DFileDialog::ExistingFiles : DFileDialog::ExistingFile);
    const QString filter = tr("Supported files") + QString(" (%1)").arg(suffixes.join(" "));
    dlg.setNameFilter(filter);
    dlg.selectNameFilter(filter);
    dlg.setObjectName("fileDialogAdd");

    if (dlg.exec() != DFileDialog::Accepted)
        return;

    m_lastImportPath = dlg.directory().path();
    const QStringList selected = dlg.selectedFiles();
    if (selected.isEmpty()) {
        qCDebug(logAIGUI) << "selectFile: no file selected";
        return;
    }

    emitIncomingFiles(selected, QString(), category, QStringLiteral("handleDroppedFiles"));
}

QString FileChannel::getFileIconBase64(const QString &filePath, int width, int height)
{
    return fileIconBase64(filePath, width, height);
}

QString FileChannel::validateIncomingPaths(const QString &params)
{
    QJsonParseError err;
    const QJsonObject root = QJsonDocument::fromJson(params.toUtf8(), &err).object();
    if (err.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "validateIncomingPaths: bad params JSON:" << err.errorString();
        return QStringLiteral(R"({"paths":[],"has_unsupported_paths":false})");
    }

    const QJsonArray pathsArray = root.value("paths").toArray();
    const bool showUnsupportedToast = root.value("show_unsupported_toast").toBool(false);

    QStringList paths;
    paths.reserve(pathsArray.size());
    for (const auto &v : pathsArray) {
        const QString path = v.toString();
        if (!path.isEmpty()) {
            paths.append(path);
        }
    }

    bool hasUnsupportedFile = false;
    const QStringList acceptedPaths = filterSupportedPaths(paths, &hasUnsupportedFile);

    if (showUnsupportedToast && hasUnsupportedFile)
        showUnsupportedFileFormatToast();

    QJsonArray acceptedPathsArray;
    for (const QString &path : acceptedPaths)
        acceptedPathsArray.append(path);

    QJsonObject result;
    result["paths"] = acceptedPathsArray;
    result["has_unsupported_paths"] = hasUnsupportedFile;
    return QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
}

void FileChannel::parseFile(const QString &id, const QString &filePath)
{
    FileService::instance()->parseFile(id, filePath);
}

void FileChannel::handleDroppedFiles(const QString &params)
{
    QJsonParseError err;
    const QJsonObject root = QJsonDocument::fromJson(params.toUtf8(), &err).object();
    if (err.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "handleDroppedFiles: bad params JSON:" << err.errorString();
        return;
    }

    const QString defaultPrompt = root.value(STR_KEY_DEFAULT_PROMPT).toString();
    const QJsonArray pathsArr   = root.value("paths").toArray();
    const int category          = root.value(STR_KEY_CATEGORY).toInt(-1);
    if (pathsArr.isEmpty()) {
        qCWarning(logAIGUI) << "handleDroppedFiles: empty paths";
        return;
    }

    QStringList paths;
    paths.reserve(pathsArr.size());
    for (const auto &v : pathsArr) {
        const QString path = v.toString();
        if (!path.isEmpty()) {
            paths.append(path);
        }
    }

    bool hasUnsupportedFile = false;
    const QStringList acceptedPaths = filterSupportedPaths(paths, &hasUnsupportedFile);
    if (hasUnsupportedFile)
        showUnsupportedFileFormatToast();

    for (const QString &path : acceptedPaths)
        processOneFile(path, defaultPrompt, FileService::supportedSuffixes(), category);
}

void FileChannel::handleCopiedFiles(const QString &params)
{
    QJsonParseError err;
    const QJsonObject root = QJsonDocument::fromJson(params.toUtf8(), &err).object();
    if (err.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "handleCopiedFiles: bad params JSON:" << err.errorString();
        return;
    }

    const int category          = root.value(STR_KEY_CATEGORY).toInt(-1);
    const QJsonArray pathsArray = root.value("paths").toArray();
    QStringList paths;
    paths.reserve(pathsArray.size());
    for (const auto &v : pathsArray) {
        const QString path = v.toString();
        if (!path.isEmpty()) {
            paths.append(path);
        }
    }

    bool hasUnsupportedFile = false;
    const QStringList acceptedPaths = filterSupportedPaths(paths, &hasUnsupportedFile);
    if (hasUnsupportedFile)
        showUnsupportedFileFormatToast();

    for (const QString &path : acceptedPaths)
        processOneFile(path, QString(), FileService::supportedSuffixes(), category);
}

void FileChannel::removeFile(const QString &filePath)
{
    FileService::instance()->evict(filePath);
}

QString FileChannel::processClipboardData()
{
    const QMimeData *mime = QApplication::clipboard()->mimeData();
    if (!mime)
        return {};

    if (mime->hasUrls()) {
        QStringList paths;
        for (const QUrl &url : mime->urls()) {
            if (url.isLocalFile())
                paths.append(url.toLocalFile());
        }
        if (!paths.isEmpty()) {
            bool hasUnsupported = false;
            const QStringList accepted = filterSupportedPaths(paths, &hasUnsupported);
            if (hasUnsupported)
                showUnsupportedFileFormatToast();
            for (const QString &path : accepted)
                processOneFile(path, QString(), FileService::supportedSuffixes());
            return {};
        }
    }

    if (mime->hasImage()) {
        const QImage img = qvariant_cast<QImage>(mime->imageData());
        if (!img.isNull()) {
            const QString tempPath = QDir::temp().filePath(
                QStringLiteral("UosAiImage-%1.jpg").arg(QDateTime::currentMSecsSinceEpoch()));
            if (img.save(tempPath, "JPEG"))
                processOneFile(tempPath, QString(), FileService::supportedSuffixes());
        }
        return {};
    }

    if (mime->hasText())
        return mime->text();

    return {};
}

int FileChannel::isEnableScreenshot()
{
    return ScreenshotService::instance()->checkAvailability();
}

void FileChannel::startScreenshot()
{
    // 写作助手不支持截图问答（包含 Ctrl+Alt+Q 全局快捷键入口）。
    if (m_currentAssistantId == QStringLiteral("uos-ai-writing")) {
        qCInfo(logAIGUI) << "Screenshot Q&A is not available in AI Writing.";
        return;
    }

    int screenshotStatus = isEnableScreenshot();
    if (screenshotStatus != 0) {
        if (screenshotStatus == 1)
            toastRequested("warning", tr("Update the UOS Screen Recorder to version 6.6 or later and restart your computer to enable Screenshot Q&A."));
        return;
    }

    emit screenshotRequested(); // 窗口隐藏、D-Bus 全交给外部
}

void FileChannel::setCurrentAssistantId(const QString &assistantId)
{
    m_currentAssistantId = assistantId;
}

bool FileChannel::isFileExist(QString filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }

    QFileInfo fileInfo(filePath);
    return fileInfo.exists() && fileInfo.isFile();
}

void FileChannel::handleScreenshotFile(const QString &params)
{
    QJsonParseError err;
    const QJsonObject root = QJsonDocument::fromJson(params.toUtf8(), &err).object();
    if (err.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "handleScreenshotFile: bad params JSON:" << err.errorString();
        return;
    }

    const QString path = root.value(STR_KEY_PATH).toString();
    if (path.isEmpty()) {
        qCWarning(logAIGUI) << "handleScreenshotFile: empty path";
        return;
    }

    bool hasUnsupportedFile = false;
    const QStringList acceptedPaths = filterSupportedPaths(QStringList{ path }, &hasUnsupportedFile);
    if (hasUnsupportedFile) {
        showUnsupportedFileFormatToast();
        return;
    }

    if (acceptedPaths.isEmpty())
        return;

    processOneFile(acceptedPaths.first(), QString(), FileService::supportedSuffixes());
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void FileChannel::emitFileReady(const QString &filePath, int error, qint64 fileSize,
                                const QString &icon, const QString &defaultPrompt, int category)
{
    QJsonObject json;
    json[STR_KEY_FILE_PATH] = filePath;
    json[STR_KEY_ERROR]     = error;
    if (fileSize >= 0)
        json[STR_KEY_FILE_SIZE] = fileSize;
    if (!icon.isEmpty())
        json[STR_KEY_ICON] = icon;
    if (!defaultPrompt.isEmpty())
        json[STR_KEY_DEFAULT_PROMPT] = defaultPrompt;
    if (category >= 0)
        json[STR_KEY_CATEGORY] = category;
    emit fileEvent(FileEvent::FeFileReady, QString(),
                   QJsonDocument(json).toJson(QJsonDocument::Compact));
}

QStringList FileChannel::filterSupportedPaths(const QStringList &paths, bool *hasUnsupported) const
{
    const QStringList allowedSuffixes = FileService::supportedSuffixes();
    QStringList acceptedPaths;
    acceptedPaths.reserve(paths.size());

    bool foundUnsupported = false;
    for (const QString &path : paths) {
        if (path.isEmpty())
            continue;

        const QString ext = QFileInfo(path).suffix().toLower();
        if (!allowedSuffixes.contains("*." + ext)) {
            foundUnsupported = true;
            qCWarning(logAIGUI) << "Unsupported suffix:" << ext;
            continue;
        }

        acceptedPaths.append(path);
    }

    if (hasUnsupported)
        *hasUnsupported = foundUnsupported;

    return acceptedPaths;
}

void FileChannel::showUnsupportedFileFormatToast()
{
    emit toastRequested("warning", tr("The file format is not supported."));
}

int FileChannel::processOneFile(const QString &filePath,
                                const QString &defaultPrompt,
                                const QStringList &allowedSuffixes,
                                int category)
{
    if (filePath.isEmpty())
        return static_cast<int>(GErrorType::NoError);

    const QFileInfo info(filePath);

    if (!info.exists()) {
        qCWarning(logAIGUI) << "File does not exist:" << filePath;
        emitFileReady(filePath, static_cast<int>(GErrorType::FileParseFailed), -1, QString(), QString(), category);
        return static_cast<int>(GErrorType::FileParseFailed);
    }

    // Size check via FileService (uses FileParser::strategyFor internally)
    const int sizeErr = FileService::instance()->checkSize(filePath);
    if (sizeErr != static_cast<int>(GErrorType::NoError)) {
        emitFileReady(filePath, sizeErr, -1, QString(), QString(), category);
        if (sizeErr == static_cast<int>(GErrorType::FileImageSizeExceeded))
            emit toastRequested("warning", tr("The image exceeds the 15 MB size limit."));
        else if (sizeErr == static_cast<int>(GErrorType::FileSizeExceeded))
            emit toastRequested("warning", tr("The file exceeds the 100 MB size limit."));
        return sizeErr;
    }

    // Suffix check (file dialog can still create files that bypass the filter)
    const QString ext = info.suffix().toLower();
    if (!allowedSuffixes.contains("*." + ext)) {
        qCWarning(logAIGUI) << "Unsupported suffix:" << ext;
        return static_cast<int>(GErrorType::FileInvalidSuffix);
    }

    emitFileReady(filePath, static_cast<int>(GErrorType::NoError), info.size(),
                  fileIconBase64(filePath), defaultPrompt, category);
    return static_cast<int>(GErrorType::NoError);
}

QString FileChannel::fileIconBase64(const QString &filePath, int width, int height)
{
    const QIcon icon = fileIcon(filePath);
    if (icon.isNull())
        return {};

    const int requestedWidth = qMax(1, width);
    const int requestedHeight = qMax(1, height);
    const qreal dpr = qApp->devicePixelRatio();
    QImage image = icon.pixmap(static_cast<int>(requestedWidth * dpr),
                               static_cast<int>(requestedHeight * dpr)).toImage();
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    return QString::fromLatin1(data.toBase64());
}

QIcon FileChannel::fileIcon(const QString &filePath)
{
    if (filePath.isEmpty())
        return {};

    QFileInfo info(filePath);
    if (!info.exists())
        return {};

    // docx/xlsx/pptx get classified as zip archives by the icon provider;
    // strip the trailing 'x' so the correct office icon is returned.
    static const QStringList stripX = { "docx", "xlsx", "pptx" };
    if (stripX.contains(info.suffix())) {
        QString stripped = filePath;
        stripped.chop(1);
        info.setFile(stripped);
    }

    DFileIconProvider provider;
    return provider.icon(info);
}
