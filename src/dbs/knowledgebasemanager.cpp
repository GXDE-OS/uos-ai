#include "knowledgebasemanager.h"
#include "embeddingserver.h"
#include "localmodelserver.h"

#include <DDialog>
#include <DLabel>
#include <report/knowledgefiletypepoint.h>
#include <report/knowledgefilenumberpoint.h>
#include <report/eventlogutil.h>

#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QDir>
#include <QDateTime>
#include <QLoggingCategory>
#include <QProcess>

using namespace uos_ai;

DWIDGET_USE_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(logKnowledgeBase)
Q_LOGGING_CATEGORY(logKnowledgeBase, "ai.knowledgebase")

const qint64 KnowledgeBaseManager::KNOWLEDGE_BASE_CAPACITY = 1024 * 1024 * 1024; // 1GB
const QStringList KnowledgeBaseManager::SUPPORTED_FORMATS = {
    "*.txt", "*.doc", "*.docx", "*.xls", "*.xlsx", "*.ppt", "*.pptx", "*.pdf", "*.md", "*.png", "*.jpeg", "*.jpg",
    "*.c", "*.cpp", "*.cxx", "*.cc", "*.c++", "*.h", "*.java", "*.py", "*.js", "*.ts", "*.rb", "*.go", "*.rs", "*.php", "*.pl", "*.pm", "*.swift",
    "*.kt", "*.kts", "*.scala", "*.sc", "*.lua", "*.r", "*.m", "*.dart", "*.html", "*.htm", "*.css", "*.jsx", "*.tsx", "*.vue",
    "*.svelte", "*.ejs", "*.pug", "*.jade", "*.handlebars", "*.hbs", "*.sh", "*.bat", "*.ps1", "*.zsh", "*.json",
    "*.xml", "*.yml", "*.yaml", "*.toml", "*.ini", "*.env", "*.sql", "*.gradle", "*.pom", "*.sln","*.csproj",
    "*.vbproj", "*.vcxproj", "*.mustache", "*.twig", "*.scss", "*.sass", "*.less", "*.jsp", "*.aspx",
    "*.erb", "*.s", "*.asm", "*.v", "*.vh", "*.vhd", "*.vhdl", "*.cs", "*.fs", "*.fsx", "*.hs", "*.d", "*.groovy",
    "*.jl", "*.elm", "*.clj", "*.cljs", "*.cljc", "*.erl", "*.hrl", "*.fsl", "*.fsi", "*.gitignore",
    "*.tf", "*.ipynb"
};

KnowledgeBaseManager& KnowledgeBaseManager::getInstance()
{
    static KnowledgeBaseManager instance;
    return instance;
}

KnowledgeBaseManager::KnowledgeBaseManager(QObject *parent)
    : QObject(parent)
{
    // 连接EmbeddingServer信号
    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::addToServerStatusChanged,
            this, &KnowledgeBaseManager::statusChanged);
    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::indexDeleted,
            this, [this](const QStringList& files) {
                for (const QString& file : files) {
                    emit fileRemoved(file);
                }
            });
}

KnowledgeBaseManager::AddResult KnowledgeBaseManager::validateFiles(const QStringList &filePaths, qint64 &totalSize)
{
    foreach(QString filePath, filePaths) {
        QFile file(filePath);
        if (!file.exists()) {
            return AddResult::FileNotFound;
        }
        if (!isSupportedFormat(filePath)) {
            return AddResult::UnsupportedFormat;
        }
        QFileInfo fileInfo(file);
        totalSize += fileInfo.size();
    }
    return AddResult::Success;
}

KnowledgeBaseManager::AddResult KnowledgeBaseManager::checkCapacityLimits(qint64 additionalSize)
{
    qint64 availableSize = getAvailableSize();
    if (availableSize < additionalSize) {
        return AddResult::InsufficientCapacity;
    }
    return AddResult::Success;
}

KnowledgeBaseManager::AddResult KnowledgeBaseManager::checkDiskSpace(qint64 requiredSize)
{
    qint64 diskSpaceSize = getDiskSpace(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (diskSpaceSize < requiredSize) {
        return AddResult::InsufficientDiskSpace;
    }
    return AddResult::Success;
}

QStringList KnowledgeBaseManager::supportedFormats()
{
    return SUPPORTED_FORMATS;
}

bool KnowledgeBaseManager::isSupportedFormat(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    QString pattern = "*." + suffix;
    return SUPPORTED_FORMATS.contains(pattern);
}

QVector<QPair<int, QString>> KnowledgeBaseManager::getKnowledgeBaseFiles()
{
    return EmbeddingServer::getInstance().getDocFiles();
}

qint64 KnowledgeBaseManager::getKnowledgeBaseCapacity()
{
    return KNOWLEDGE_BASE_CAPACITY;
}

qint64 KnowledgeBaseManager::getCurrentUsedSize()
{
    qint64 totalSize = 0;
    auto documents = EmbeddingServer::getInstance().getDocFiles();

    for (const auto& doc : documents) {
        QFileInfo fileInfo(doc.second);
        if (fileInfo.exists()) {
            totalSize += fileInfo.size();
        }
    }

    return totalSize;
}

qint64 KnowledgeBaseManager::getAvailableSize()
{
    return KNOWLEDGE_BASE_CAPACITY - getCurrentUsedSize();
}

qint64 KnowledgeBaseManager::getDiskSpace(const QString& mountPoint)
{
    QProcess process;
    QStringList arguments;
    arguments << mountPoint;
    process.start("df", arguments);
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList fields;

#ifdef COMPILE_ON_QT6
    QStringList lines = output.split(QRegularExpression("\\n"), Qt::SkipEmptyParts);
    if (lines.size() > 1)
        fields = lines.at(1).split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
#else
    QStringList lines = output.split(QRegExp("\\n"), QString::SkipEmptyParts);
    if (lines.size() > 1)
        fields = lines.at(1).split(QRegExp("\\s+"), QString::SkipEmptyParts);
#endif

    if (fields.size() >= 5) {
        // 通常df的输出格式是：文件系统 容量 已用 可用 已用% 挂载点
        return fields.at(3).toLongLong() * 1024;
    }

    return -1;
}

QString KnowledgeBaseManager::formatSize(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;

    if (bytes >= GB) {
        return QString::number(bytes / GB) + "GB";
    } else if (bytes >= MB) {
        return QString::number(bytes / MB) + "MB";
    } else if (bytes >= KB) {
        return QString::number(bytes / KB) + "KB";
    } else {
        return QString::number(bytes) + "B";
    }
}

KnowledgeBaseManager::AddFileResult KnowledgeBaseManager::addFilesToKnowledgeBase(const AddFileRequest& request)
{
    AddFileResult result;
    result.totalSize = 0;  // 明确初始化totalSize

    // 检查embedding插件状态
    if (!LocalModelServer::getInstance().checkInstallStatus(PLUGINSNAME)) {
        result.result = AddResult::EmbeddingPluginNotAvailable;
        showInstallModelDialog();
        return result;
    }

    QStringList filesToProcess;

    if (request.source == FileSource::TemporaryText) {
        // 创建临时文本文件
        QString tempFilePath = createTemporaryTextFile(request.textContent, request.fileName);
        if (tempFilePath.isEmpty()) {
            result.result = AddResult::FileNotFound;
            return result;
        }
        filesToProcess << tempFilePath;
    } else {
        filesToProcess = request.filePaths;
    }

    // 验证文件
    qint64 totalSize = 0;
    AddResult validationResult = validateFiles(filesToProcess, totalSize);
    if (validationResult != AddResult::Success) {
        result.result = validationResult;
        cleanupTemporaryFiles();
        return result;
    }

    result.totalSize = totalSize;

    // 检查容量限制
    AddResult capacityResult = checkCapacityLimits(totalSize);
    if (capacityResult != AddResult::Success) {
        result.result = capacityResult;
        cleanupTemporaryFiles();
        showInsufficientCapacityDialog(getCurrentUsedSize(), totalSize);
        return result;
    }

    // 检查磁盘空间
    AddResult diskSpaceResult = checkDiskSpace(totalSize);
    if (diskSpaceResult != AddResult::Success) {
        result.result = diskSpaceResult;
        cleanupTemporaryFiles();
        showInsufficientDiskSpaceDialog(totalSize);
        return result;
    }

    Q_EMIT filesProcessing(filesToProcess);

    // 创建向量索引
    QStringList successFiles = createVectorIndexes(filesToProcess);

    result.result = AddResult::Success;
    result.successFiles = successFiles;

    // 计算失败的文件
    for (const QString& file : filesToProcess) {
        if (!successFiles.contains(file)) {
            result.failedFiles << file;
        }
    }

    // 发射信号
    emit capacityChanged(getCurrentUsedSize(), KNOWLEDGE_BASE_CAPACITY);
    ReportIns()->writeEvent(report::KnowledgeFileNumberPoint(getKnowledgeBaseFiles().size()).assemblingData());

    return result;
}

KnowledgeBaseManager::AddFileResult KnowledgeBaseManager::addExistingFiles(const QStringList& filePaths)
{
    AddFileRequest request;
    request.filePaths = filePaths;
    request.source = FileSource::ExistingFile;

    return addFilesToKnowledgeBase(request);
}

KnowledgeBaseManager::AddFileResult KnowledgeBaseManager::addTextAsFile(const QString& textContent, const QString& fileName)
{
    AddFileRequest request;
    request.textContent = textContent;
    if (!fileName.isEmpty())
        request.fileName = fileName;
    else
        request.fileName = tr("FollowAlong") + "_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    request.source = FileSource::TemporaryText;

    return addFilesToKnowledgeBase(request);
}

void KnowledgeBaseManager::showInsufficientCapacityDialog(qint64 currentSize, qint64 additionalSize)
{
    qCWarning(logKnowledgeBase) << "Insufficient knowledge base capacity. Current size:" << currentSize << ", Adding size:" << additionalSize;
    DDialog dialog(tr("Insufficient knowledge base capacity"),
                   tr("The total capacity of the knowledge base is %1M, with a remaining %2. "
                    "The total number of files added this time is %3. Unable to complete the add to knowledge base operation.")
                    .arg(1024).arg(formatSize(KNOWLEDGE_BASE_CAPACITY - currentSize)).arg(formatSize(additionalSize)));
    dialog.setWordWrapMessage(true);
    dialog.setFixedWidth(380);
    dialog.setIcon(QIcon(":assets/images/warning.svg"));
    dialog.addButton(tr("OK", "button"));
    auto labelList = dialog.findChildren<QLabel *>();
    for (auto messageLabel : labelList) {
        if ("MessageLabel" == messageLabel->objectName()) {
            messageLabel->setFixedWidth(dialog.width() - 20);
        }
    }
    int ret = dialog.exec();
}

void KnowledgeBaseManager::showInsufficientDiskSpaceDialog(qint64 requiredSize)
{
    qint64 diskSpaceSize = getDiskSpace(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    qCWarning(logKnowledgeBase) << "Not enough disk space. Required:" << requiredSize << ", Available:" << diskSpaceSize;
    DDialog dialog(tr("Not enough disk space") ,
                    tr("To store the newly added files, at least %1 of disk space is required. The current remaining space is %2. "
                        "Please clear enough hard disk space and try again.").arg(formatSize(requiredSize)).arg(formatSize(diskSpaceSize)));
    dialog.setIcon(QIcon(":assets/images/warning.svg"));
    dialog.setWordWrapMessage(true);
    dialog.setFixedWidth(380);
    dialog.addButton(QObject::tr("OK", "button"));
    auto labelList = dialog.findChildren<QLabel *>();
    for (auto messageLabel : labelList) {
        if ("MessageLabel" == messageLabel->objectName()) {
            messageLabel->setFixedWidth(dialog.width() - 20);
        }
    }
    int ret = dialog.exec();
}

void KnowledgeBaseManager::showInstallModelDialog()
{
    qCWarning(logKnowledgeBase) << "Vectorization model plugin not installed";
    DDialog dialog("",tr("Adding to the knowledge base requires installing the vectorization model plugin. Please go to the app store to download and install."));
    dialog.setFixedWidth(380);
    dialog.setIcon(QIcon(":assets/images/warning.svg"));
    auto labelList = dialog.findChildren<QLabel *>();
    for (auto messageLabel : labelList) {
        if ("MessageLabel" == messageLabel->objectName())
            messageLabel->setFixedWidth(dialog.width() - 20);
    }
    dialog.addButton(tr("Do not install", "button"), true, DDialog::ButtonNormal);
    dialog.addButton(tr("Install immediately", "button"), true, DDialog::ButtonRecommend);
    if (DDialog::Accepted == dialog.exec()) {
        qCInfo(logKnowledgeBase) << "User chose to install vectorization plugin";
        LocalModelServer::getInstance().openInstallWidgetOnTimer(PLUGINSNAME);
    }
}

bool KnowledgeBaseManager::removeFromKnowledgeBase(const QStringList& fileNames)
{
    return EmbeddingServer::getInstance().deleteVectorIndex(fileNames);
}

QString KnowledgeBaseManager::createTemporaryTextFile(const QString& textContent, const QString& fileName)
{
    // 创建临时目录
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/uos-ai/knowledge-base";
    QDir().mkpath(tempDir);

    // 创建临时文件
    QTemporaryFile* tempFile = new QTemporaryFile(tempDir + "/" + fileName + "_XXXXXX.txt");
    tempFile->setAutoRemove(false); // 我们手动管理删除

    if (!tempFile->open()) {
        qCWarning(logKnowledgeBase) << "Failed to create temporary file for text content";
        delete tempFile;
        return QString();
    }

    // 写入文本内容
    QTextStream stream(tempFile);
#if COMPILE_ON_QT6
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
    stream << textContent;
    tempFile->close();

    QString filePath = tempFile->fileName();
    m_temporaryFiles.append(tempFile);

    qCDebug(logKnowledgeBase) << "Created temporary text file:" << filePath;
    return filePath;
}

QStringList KnowledgeBaseManager::createVectorIndexes(const QStringList &filePaths)
{
    for (const QString &file : filePaths) {
        QFileInfo fileInfo(file);
        ReportIns()->writeEvent(report::KnowledgeFileTypePoint(fileInfo.suffix().toLower()).assemblingData());
    }
    QStringList createdList {};
    for (const QString &file : filePaths) {
        qCDebug(logKnowledgeBase) << "Creating vector index for file:" << file;
        if (EmbeddingServer::getInstance().createVectorIndex(QStringList(file)))
            createdList << file;
    }
    return createdList;
}

void KnowledgeBaseManager::cleanupTemporaryFiles()
{
    for (QTemporaryFile* file : m_temporaryFiles) {
        if (file->exists()) {
            file->remove();
        }
        file->deleteLater();
    }
    m_temporaryFiles.clear();
}
