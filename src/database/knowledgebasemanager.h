
#ifndef KNOWLEDGEBASEMANAGER_H
#define KNOWLEDGEBASEMANAGER_H

#include <QObject>
#include <QStringList>
#include <QTemporaryFile>
#include <QFileInfo>

class EmbeddingServer;
class KnowledgeBaseManager : public QObject
{
    Q_OBJECT

public:
    enum class AddResult {
        Success,
        FileNotFound,
        UnsupportedFormat,
        FileExists,
        InsufficientCapacity,
        InsufficientDiskSpace,
        EmbeddingPluginNotAvailable,
        UnknownError
    };

    enum class FileSource {
        ExistingFile,    // 现有文件
        TemporaryText    // 临时文本文件
    };

    struct AddFileRequest {
        QStringList filePaths;
        FileSource source = FileSource::ExistingFile;
        QString textContent;  // 当source为TemporaryText时使用
        QString fileName;     // 临时文件的文件名
    };

    struct AddFileResult {
        AddResult result;
        QString errorMessage;
        QStringList successFiles;
        QStringList failedFiles;
        qint64 totalSize = 0;
    };

    static KnowledgeBaseManager& getInstance();
    
    // 支持的文件格式
    QStringList supportedFormats();
    bool isSupportedFormat(const QString& filePath);

    // 文件管理
    QVector<QPair<int, QString>> getKnowledgeBaseFiles();
    
    // 容量管理
    qint64 getKnowledgeBaseCapacity();
    qint64 getCurrentUsedSize();
    qint64 getAvailableSize();
    qint64 getDiskSpace(const QString& mountPoint);
    
    // 核心添加方法
    AddFileResult addFilesToKnowledgeBase(const AddFileRequest& request);
    AddFileResult addExistingFiles(const QStringList& filePaths);
    AddFileResult addTextAsFile(const QString& textContent, const QString& fileName = "");

    void showInsufficientCapacityDialog(qint64 currentSize, qint64 additionalSize);
    void showInsufficientDiskSpaceDialog(qint64 requiredSize);
    void showInstallModelDialog();
    
    // 删除方法
    bool removeFromKnowledgeBase(const QStringList& fileNames);
    
    // 工具方法
    static QString formatSize(qint64 bytes);

private:
    explicit KnowledgeBaseManager(QObject *parent = nullptr);
    
    // 验证方法
    AddResult validateFiles(const QStringList& filePaths, qint64& totalSize);
    AddResult checkCapacityLimits(qint64 additionalSize);
    AddResult checkDiskSpace(qint64 requiredSize);
    
    // 文件处理
    QString createTemporaryTextFile(const QString& textContent, const QString& fileName);
    QStringList createVectorIndexes(const QStringList& filePaths);
    void cleanupTemporaryFiles();
    
    // 常量
    static const qint64 KNOWLEDGE_BASE_CAPACITY; // 1GB
    static const QStringList SUPPORTED_FORMATS;
    
    // 成员变量
    QList<QTemporaryFile*> m_temporaryFiles;
    
signals:
    void filesProcessing(const QStringList& files);
    void fileAdded(const QString& filePath, int status);
    void fileRemoved(const QString& filePath);
    void statusChanged(const QStringList& files, int status);
    void capacityChanged(qint64 usedSize, qint64 totalCapacity);
};

#endif // KNOWLEDGEBASEMANAGER_H
