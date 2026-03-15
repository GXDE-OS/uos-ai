#ifndef EPARSERDOCUMENT_H
#define EPARSERDOCUMENT_H

#include "serverdefs.h"
#include <QObject>
#include <QProcess>
#include <QFileDialog>

namespace uos_ai {
class EParserDocument : public QObject
{
    Q_OBJECT
public:
    static EParserDocument *instance();

    enum ParserStart {
        Success = 0,
        FileCountError,
        SuffixError,
        NoDocError,
        ExceedSize,
        ImageExceedSize,
    };

    enum ParserError {
        NoError = 0,
        ParsingFailed,
        NoTextError,
        NoTextExtracted
    };

    enum FileType {
        Document = 0,
        Image    = 1,
    };

    enum FileCategory {
        LocalMaterial = 0,
        FileOutline = 1
    };

    void parser(const QString &id, const QString &docPath);
    QString parserSync(const QString &docPath);
    void selectDocument(AssistantType type, QWidget *parent);
    void selectDocumentForOffice(FileCategory category, QWidget *parent);
    void dragInViewDocument(const QStringList &docPaths, const QString &defaultPrompt, AssistantType type);
    void dragInViewDocForWriting(const QStringList &docPaths);
    void handleScreenshotImage(const QString &imagePath, AssistantType type);
    void handleCopyFile(const QStringList &filePaths, AssistantType type);
signals:
    void sigSelectFilesForOffice(const QString filesDataJson, int error, FileCategory category);
    void sigParserStart(const QString &docPath, const QString &iconData, const QString &defaultPrompt, int error);
    void sigParserResult(const QString &id, int error, const QString &docPath, const QString &docContent);

private:
    using ParserRes = QPair<QString, ParserError>;
    explicit EParserDocument();
    ParserRes parserDoc(const QString &docPath);
    ParserRes parserImg(const QString &imgPath);

    bool validSize(const QString &docPath);
    QString getFileIconData(const QString &docPath);
    QIcon getFileIcon(const QString &docPath);
    void updateSupSuffix(AssistantType type);

    QString runOCRProcessByPath(QStringList imagePathList);
    QString runOCRProcessByImage(QStringList imageList);
    QString runOCRProcessTool(QProcess *ocrProcess, const QStringList &arguments, const QByteArray &inputData = QByteArray());

    FileType getFileType(const QString &filePath);

    bool execFileSelect(QStringList &paths, QFileDialog::FileMode mode = QFileDialog::ExistingFile, QWidget *parent = nullptr);
private:
    QString m_lastImportPath;
    QStringList m_supSuffix;
};
}
#endif // EPARSERDOCUMENT_H
