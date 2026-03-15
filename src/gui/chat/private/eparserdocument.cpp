#include "eparserdocument.h"
#include "filetypeselectdialog.h"
#include "../parsers/parserfactory.h"

#include <DFileDialog>
#include <DFileIconProvider>

#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QApplication>
#include <QLoggingCategory>

#include <docparser.h>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

UOSAI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

EParserDocument *EParserDocument::instance()
{
    static EParserDocument ins;
    return &ins;
}

void EParserDocument::parser(const QString &id, const QString &docPath)
{    
    QFutureWatcher<ParserRes> *futureWatcher = new QFutureWatcher<ParserRes>;
    QFuture<ParserRes> future = QtConcurrent::run([this, docPath]{
        ParserRes res;
        FileType fileType = getFileType(docPath);
        switch (fileType) {
        case FileType::Image:
            res = this->parserImg(docPath);
            break;
        case FileType::Document:
            res = this->parserDoc(docPath);
            break;
        }
        return res;
    });

    QObject::connect(futureWatcher, &QFutureWatcher<ParserRes>::finished, this, [=]{
        ParserRes parserRes = futureWatcher->future().result();
        emit sigParserResult(id, parserRes.second, docPath, parserRes.first);
        futureWatcher->deleteLater();
    });
    futureWatcher->setFuture(future);
}

QString EParserDocument::parserSync(const QString &docPath)
{
    ParserRes res;
    FileType fileType = getFileType(docPath);
    switch (fileType) {
    case FileType::Image:
        res = this->parserImg(docPath);
        break;
    case FileType::Document:
        res = this->parserDoc(docPath);
        break;
    }
    return res.first;
}

void EParserDocument::selectDocument(AssistantType type, QWidget *parent)
{
    DFileDialog fileDlg(parent);
    fileDlg.setDirectory(m_lastImportPath);
    updateSupSuffix(type);
    QString selfilter = tr("Supported files") + (" (%1)");
    selfilter = selfilter.arg(m_supSuffix.join(" "));
    fileDlg.setViewMode(DFileDialog::Detail);
    fileDlg.setFileMode(DFileDialog::ExistingFile);
    fileDlg.setNameFilter(selfilter);
    fileDlg.selectNameFilter(selfilter);
    fileDlg.setObjectName("fileDialogAdd");
    if (DFileDialog::Accepted == fileDlg.exec()) {
        m_lastImportPath = fileDlg.directory().path();
        QStringList selectedPaths = fileDlg.selectedFiles();
        if (selectedPaths.isEmpty()) {
            qCDebug(logAIGUI) << "No file selected in dialog";
            return;
        }

        if (!validSize(selectedPaths[0])) {
            qCWarning(logAIGUI) << "Selected file exceeds size limit:" << selectedPaths[0];
            return;
        }

        // 文件选择框中创建新文件不会过滤，需要再判断一次后缀
        QFileInfo docInfo(selectedPaths[0]);
        if (!m_supSuffix.contains("*." + docInfo.suffix().toLower())) {
            qCWarning(logAIGUI) << "Unsupported document suffix:" << docInfo.suffix();
            emit sigParserStart(QString(), QString(), QString(), ParserStart::SuffixError);
            return;
        }

        emit sigParserStart(selectedPaths[0], getFileIconData(selectedPaths[0]), QString(), ParserStart::Success);
    }
}

void EParserDocument::selectDocumentForOffice(FileCategory category, QWidget *parent)
{
    updateSupSuffix(AssistantType::AI_WRITING);

    QStringList selectedPaths {};
    if (!execFileSelect(selectedPaths, category == FileCategory::LocalMaterial ? QFileDialog::ExistingFiles : QFileDialog::ExistingFile, parent))
        return;

    QJsonArray filesArray;
    // 文件选择框中创建新文件不会过滤，需要再判断一次后缀
    for (auto filePath : selectedPaths) {
        QFileInfo docInfo(filePath);
        if (!m_supSuffix.contains("*." + docInfo.suffix().toLower())) {
            qCWarning(logAIGUI) << "Unsupported document suffix:" << docInfo.suffix();
            continue;
        }

        QString icon = getFileIconData(filePath);
        // 创建文件信息JSON对象
        QJsonObject fileObj;
        fileObj["filePath"] = filePath;
        fileObj["fileIcon"] = icon;

        filesArray.append(fileObj);
    }

    if (filesArray.isEmpty()) {// 创建空的JSON数组
        QJsonDocument emptyDoc(filesArray);
        emit sigSelectFilesForOffice(emptyDoc.toJson(QJsonDocument::Compact), ParserStart::SuffixError, category);
        return;
    }

    // 将JSON数组转换为字符串
    QJsonDocument doc(filesArray);
    QString filesDataJson = doc.toJson(QJsonDocument::Compact);
    emit sigSelectFilesForOffice(filesDataJson, ParserStart::Success, category);
}

void EParserDocument::updateSupSuffix(AssistantType type)
{
    if (type == AssistantType::PLUGIN_ASSISTANT) {
        m_supSuffix.clear();
        m_supSuffix << "*.docx";
    } else {
        m_supSuffix.clear();
        m_supSuffix << "*.txt" << "*.doc" << "*.docx" << "*.xls" << "*.xlsx" << "*.ppt" << "*.pptx" 
                << "*.pdf" << "*.md" << "*.png" << "*.jpeg" << "*.jpg" << "*.c" << "*.cpp" << "*.cxx" << "*.cc" << "*.c++"
                << "*.h" << "*.java" << "*.py" << "*.js" << "*.ts" << "*.rb" << "*.go" << "*.rs" 
                << "*.php" << "*.pl" << "*.pm" << "*.swift" << "*.kt" << "*.kts" << "*.scala" 
                << "*.sc" << "*.lua" << "*.r" << "*.m" << "*.dart" << "*.html" << "*.css" 
                << "*.jsx" << "*.tsx" << "*.vue" << "*.svelte" << "*.ejs" << "*.pug" << "*.jade" 
                << "*.handlebars" << "*.hbs" << "*.sh" << "*.bat" << "*.ps1" << "*.zsh"
                << "*.json" << "*.xml" << "*.yml" << "*.yaml" << "*.toml" << "*.ini" << "*.env"
                << "*.sql" << "*.gradle" << "*.pom" << "*.sln" << "*.csproj"
                << "*.vbproj" << "*.vcxproj" << "*.mustache" << "*.twig" << "*.scss" << "*.sass" 
                << "*.less" << "*.jsp" << "*.aspx" << "*.erb" << "*.s" << "*.asm" << "*.v" 
                << "*.vh" << "*.vhd" << "*.vhdl" << "*.cs" << "*.fs" << "*.fsx" << "*.hs" 
                << "*.d" << "*.groovy" << "*.jl" << "*.elm" << "*.clj" << "*.cljs" << "*.cljc" 
                << "*.erl" << "*.hrl" << "*.fsl" << "*.fsi" << "*.gitignore" << "*.tf" << "*.ipynb";
    }
}

void EParserDocument::dragInViewDocument(const QStringList &docPaths, const QString &defaultPrompt, AssistantType type)
{
    if (docPaths.size() == 0) {
        qCWarning(logAIGUI) << "No document provided";
        emit sigParserStart(QString(), QString(), QString(), ParserStart::NoDocError);
        return;
    }

    for (const QString &path : docPaths) {
        if (!validSize(path)) {
            qCWarning(logAIGUI) << "Document exceeds size limit:" << path;
            continue;
        }

        QFileInfo docInfo(path);
        updateSupSuffix(type);
        if (!m_supSuffix.contains("*." + docInfo.suffix().toLower())) {
            qCWarning(logAIGUI) << "Unsupported document suffix:" << docInfo.suffix();
            emit sigParserStart(QString(), QString(), QString(), ParserStart::SuffixError);
            continue;
        }

        emit sigParserStart(path, getFileIconData(path), defaultPrompt, ParserStart::Success);
    }
}

void EParserDocument::dragInViewDocForWriting(const QStringList &docPaths)
{
    updateSupSuffix(AssistantType::AI_WRITING);

    QJsonArray filesArray {};
    QList<QIcon> iconList {};
    // 文件选择框中创建新文件不会过滤，需要再判断一次后缀
    for (auto filePath : docPaths) {
        QFileInfo docInfo(filePath);
        if (!m_supSuffix.contains("*." + docInfo.suffix().toLower())) {
            qCWarning(logAIGUI) << "Unsupported document suffix:" << docInfo.suffix();
            continue;
        }

        QString icon = getFileIconData(filePath);
        // 创建文件信息JSON对象
        QJsonObject fileObj;
        fileObj["filePath"] = filePath;
        fileObj["fileIcon"] = icon;

        filesArray.append(fileObj);

        if (iconList.length() < 5) {
            iconList.append(getFileIcon(filePath));
        }
    }

    if (filesArray.empty()) { // 无支持的文件
        // 通知前端，且不报错
        emit sigParserStart(QString(), QString(), QString(), ParserStart::Success);
        return;
    }

    // 显示文件类型选择对话框
    FileTypeSelectDialog *dialog = new FileTypeSelectDialog(iconList, filesArray.size());
    connect(dialog, &FileTypeSelectDialog::categorySelected, this, [this, filesArray](EParserDocument::FileCategory category) {
        QJsonDocument doc(filesArray);
        QString filesDataJson = doc.toJson(QJsonDocument::Compact);
        emit sigSelectFilesForOffice(filesDataJson, ParserStart::Success, category);
    });
    connect(dialog, &FileTypeSelectDialog::finished, dialog, &FileTypeSelectDialog::deleteLater);

    int ret = dialog->exec();
    if (ret == QDialog::Rejected) {
        // 拒绝直接通知前端，且不报错
        emit sigSelectFilesForOffice(QString(), ParserStart::Success, FileCategory::LocalMaterial);
    }
}

void EParserDocument::handleScreenshotImage(const QString &imagePath, AssistantType type)
{
    if (!validSize(imagePath)) {
        qCWarning(logAIGUI) << "Document exceeds size limit:" << imagePath;
        return;
    }

    QFileInfo imageInfo(imagePath);
    updateSupSuffix(type);
    if (!m_supSuffix.contains("*." + imageInfo.suffix().toLower())) {
        qCWarning(logAIGUI) << "Unsupported document suffix:" << imageInfo.suffix();
        emit sigParserStart(QString(), QString(), QString(), ParserStart::SuffixError);
        return;
    }

    emit sigParserStart(imagePath, getFileIconData(imagePath), QString(), ParserStart::Success);
}

void EParserDocument::handleCopyFile(const QStringList &filePaths, AssistantType type)
{
    for (const QString &path : filePaths) {
        if (!validSize(path)) {
            qCWarning(logAIGUI) << "Document exceeds size limit:" << path;
            continue;
        }

        QFileInfo docInfo(path);
        updateSupSuffix(type);
        if (!m_supSuffix.contains("*." + docInfo.suffix().toLower())) {
            qCWarning(logAIGUI) << "Unsupported document suffix:" << docInfo.suffix();
            emit sigParserStart(QString(), QString(), QString(), ParserStart::SuffixError);
            continue;
        }

        emit sigParserStart(path, getFileIconData(path), QString(), ParserStart::Success);
    }
}

EParserDocument::EParserDocument()
{
    m_lastImportPath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();

    m_supSuffix << "*.txt" << "*.doc" << "*.docx" << "*.xls" << "*.xlsx" << "*.ppt" << "*.pptx"
                << "*.pdf" << "*.md" << "*.png" << "*.jpeg" << "*.jpg" << "*.c" << "*.cpp" << "*.cxx" << "*.cc" << "*.c++"
                << "*.h" << "*.java" << "*.py" << "*.js" << "*.ts" << "*.rb" << "*.go" << "*.rs"
                << "*.php" << "*.pl" << "*.pm" << "*.swift" << "*.kt" << "*.kts" << "*.scala"
                << "*.sc" << "*.lua" << "*.r" << "*.m" << "*.dart" << "*.html" << "*.css"
                << "*.jsx" << "*.tsx" << "*.vue" << "*.svelte" << "*.ejs" << "*.pug" << "*.jade"
                << "*.handlebars" << "*.hbs" << "*.sh" << "*.bat" << "*.ps1" << "*.zsh"
                << "*.json" << "*.xml" << "*.yml" << "*.yaml" << "*.toml" << "*.ini" << "*.env"
                << "*.sql" << "*.gradle" << "*.pom" << "*.sln" << "*.csproj"
                << "*.vbproj" << "*.vcxproj" << "*.mustache" << "*.twig" << "*.scss" << "*.sass"
                << "*.less" << "*.jsp" << "*.aspx" << "*.erb" << "*.s" << "*.asm" << "*.v"
                << "*.vh" << "*.vhd" << "*.vhdl" << "*.cs" << "*.fs" << "*.fsx" << "*.hs"
                << "*.d" << "*.groovy" << "*.jl" << "*.elm" << "*.clj" << "*.cljs" << "*.cljc"
                << "*.erl" << "*.hrl" << "*.fsl" << "*.fsi" << "*.gitignore" << "*.tf" << "*.ipynb";
}

EParserDocument::ParserRes EParserDocument::parserDoc(const QString &docPath)
{
    QSharedPointer<AbstractFileParser> parser(ParserFactory::instance()->createParserForFile(docPath));
    if (!parser) {
        qCWarning(logAIGUI) << "file parser create failed, don't support file:" << docPath;
        return qMakePair(QString(), ParserError::NoTextError);
    }

    QSharedPointer<ParserResult> parseResult = parser->parseFile(docPath);
    if (parseResult.isNull() || parseResult->status() != 0) {
        QString errorMsg = parseResult.isNull() ? "Parse result is null" : parseResult->errorString();
        qCWarning(logAIGUI) << "Failed to parse document:" << docPath << "Error:" << errorMsg;

        return qMakePair(QString(), ParserError::NoTextError);
    }

    QString extractedText = parseResult->getAllText();
    if (extractedText.isEmpty()) {
        qCWarning(logAIGUI) << "No text content extracted from file:" << docPath;
        return qMakePair(QString(), ParserError::NoTextError);
    }

    qCDebug(logAIGUI) << "Document parsed successfully:" << docPath;
    return qMakePair(extractedText, ParserError::NoError);
}

EParserDocument::ParserRes EParserDocument::parserImg(const QString &imgPath)
{
    // parser image from imgPath
    QStringList imagePathList;
    imagePathList.append(imgPath);
    QString contents = runOCRProcessByPath(imagePathList);
    if (contents.isEmpty()) {
        qCWarning(logAIGUI) << "Image parsing failed: No text content found in" << imgPath;
        return qMakePair(QString(), ParserError::NoTextExtracted);
    }
    return qMakePair(contents, ParserError::NoError);
}

bool EParserDocument::validSize(const QString &docPath)
{
    QFileInfo docInfo(docPath);

    FileType fileType = getFileType(docPath);
    switch (fileType) {
    case FileType::Image:
        if (docInfo.size() / 1024.0 / 1024.0 > 15.0) {
            emit sigParserStart(QString(), QString(), QString(), ParserStart::ImageExceedSize);
            return false;
        } else {
            break;
        }
    case FileType::Document:
        if (docInfo.size() / 1024.0 / 1024.0 > 100.0) {
            emit sigParserStart(QString(), QString(), QString(), ParserStart::ExceedSize);
            return false;
        } else {
            break;
        }
    }

    return true;
}

QString EParserDocument::getFileIconData(const QString &docPath)
{
    QIcon icon = getFileIcon(docPath);
    if (icon.isNull())
        return "";

    int size = static_cast<int>(qApp->devicePixelRatio() * 32);
    QImage image = icon.pixmap(size, size).toImage();
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    return QString::fromLatin1(data.toBase64());
}

QIcon EParserDocument::getFileIcon(const QString &docPath)
{
    if (docPath.isEmpty())
        return QIcon();

    QFileInfo docInfo(docPath);
    if (!docInfo.exists())
        return QIcon();

    QStringList suffixReplace;
    suffixReplace << "docx"
                  << "xlsx"
                  << "pptx";

    if (suffixReplace.contains(docInfo.suffix())) {
        // 文档后缀带x会获取到压缩包图标，需要去掉x: *.docx -> *.doc
        QString replaceXDocPath = docPath;
        replaceXDocPath.remove(replaceXDocPath.size() - 1, 1);
        docInfo.setFile(replaceXDocPath);
    }

    DFileIconProvider docIcon;
    QIcon icon = docIcon.icon(docInfo);
    return icon;
}

QString EParserDocument::runOCRProcessTool(QProcess *ocrProcess, const QStringList &arguments, const QByteArray &inputData)
{
    // 用于收集所有OCR结果的字符串
    QString allResults;
    QMutex resultsMutex;

    // 设置进程完成时的处理
    connect(ocrProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, ocrProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)

        qCInfo(logAIGUI) << "OCR process finished with exit code:" << exitCode;

        // 清理进程对象防止内存泄漏
        ocrProcess->deleteLater();
    });

    // 设置进程错误处理
    connect(ocrProcess, &QProcess::errorOccurred,
            [this, ocrProcess](QProcess::ProcessError error) {
        qCWarning(logAIGUI) << "OCR process error occurred:" << error;

        // 清理进程对象防止内存泄漏
        ocrProcess->deleteLater();
    });

    // 设置标准输出读取处理（实时处理输出）
    connect(ocrProcess, &QProcess::readyReadStandardOutput,
            [this, ocrProcess, &allResults, &resultsMutex]() {
        QByteArray data = ocrProcess->readAllStandardOutput();
        QStringList lines = QString::fromUtf8(data).split('\n');

        for (const QString &line : lines) {
            if (line.trimmed().isEmpty()) continue;

            // 解析 JSON 输出
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &error);

            if (error.error != QJsonParseError::NoError) {
                qCWarning(logAIGUI) << "Failed to parse OCR output JSON:" << error.errorString();
                continue;
            }

            QJsonObject obj = doc.object();
            QString type = obj["type"].toString();

            if (type == "single_result") {
                // 单个图片结果
                QJsonObject data = obj["data"].toObject();
                bool success = data["success"].toBool();
                QString result = data["result"].toString();
                QString errorMsg = data["error"].toString();

                if (success) {
                    qCInfo(logAIGUI) << "OCR result :" << result;
                    
                    // 线程安全地累加结果
                    QMutexLocker locker(&resultsMutex);
                    if (!allResults.isEmpty()) {
                        allResults += "\n\n";
                    }
                    allResults += result;
                } else {
                    qCWarning(logAIGUI) << "OCR failed :" << errorMsg;
                }

            } else if (type == "progress") {
                // 进度信息
                int processed = obj["processed"].toInt();
                int total = obj["total"].toInt();
                int percentage = obj["percentage"].toInt();

                qCInfo(logAIGUI) << "OCR progress:" << processed << "/" << total
                                 << "(" << percentage << "%)";

            }
        }
    });

    // 设置标准错误输出读取处理
    connect(ocrProcess, &QProcess::readyReadStandardError,
            [this, ocrProcess]() {
        QByteArray data = ocrProcess->readAllStandardError();
        QString errorOutput = QString::fromUtf8(data).trimmed();
        if (!errorOutput.isEmpty()) {
            // 过滤掉一些常见的非关键警告信息
            if (!errorOutput.contains("QML") &&
                    !errorOutput.contains("libpng warning") &&
                    !errorOutput.contains("deprecated")) {
                qCWarning(logAIGUI) << "OCR process error output:" << errorOutput;
            } else {
                qCDebug(logAIGUI) << "OCR process debug output:" << errorOutput;
            }
        }
    });

    // 设置 OCR 进程和参数（调试的时候可以手动切换到build路径）
    QString program = "/usr/lib/uos-ai-assistant/uos-ai-ocr-process";

    qCInfo(logAIGUI) << "Starting OCR process with program:" << program
                     << "arguments:" << arguments;

    // 启动进程
    ocrProcess->start(program, arguments);

    // 检查进程是否启动成功
    if (!ocrProcess->waitForStarted(5000)) { // 5 秒超时
        qCWarning(logAIGUI) << "Failed to start OCR process:" << ocrProcess->errorString();
        ocrProcess->deleteLater();
        return "";
    }

    // 如果有输入数据，写入到进程
    if (!inputData.isEmpty()) {
        qCInfo(logAIGUI) << "Sending data to OCR process";
        ocrProcess->write(inputData);
        ocrProcess->closeWriteChannel(); // 关闭写入通道，表示数据发送完毕
    }

    qCInfo(logAIGUI) << "OCR process started successfully";

    int ocrmsecs = 900000;// 15分钟超时
    if (!ocrProcess->waitForFinished(ocrmsecs)) {
        qCWarning(logAIGUI) << "Failed to finish OCR process:" << ocrProcess->errorString();
        ocrProcess->deleteLater();
        return "";
    }

    // 返回累加的所有结果
    QMutexLocker locker(&resultsMutex);
    qCInfo(logAIGUI) << "OCR process completed, returning accumulated results";
    return allResults;
}

bool EParserDocument::execFileSelect(QStringList &paths, QFileDialog::FileMode mode, QWidget *parent)
{
    DFileDialog fileDlg(parent);
    fileDlg.setDirectory(m_lastImportPath);
    QString selfilter = tr("Supported files") + (" (%1)");
    selfilter = selfilter.arg(m_supSuffix.join(" "));
    fileDlg.setViewMode(DFileDialog::Detail);
    fileDlg.setFileMode(mode);
    fileDlg.setNameFilter(selfilter);
    fileDlg.selectNameFilter(selfilter);
    fileDlg.setObjectName("fileDialogAdd");
    if (DFileDialog::Accepted == fileDlg.exec()) {
        m_lastImportPath = fileDlg.directory().path();
        paths = fileDlg.selectedFiles();
    }

    if (paths.isEmpty()) {
        qCDebug(logAIGUI) << "No file selected in dialog";
        return false;
    }

    if (!validSize(paths[0])) {
        qCWarning(logAIGUI) << "Selected file exceeds size limit:" << paths[0];
        return false;
    }

    return true;
}

EParserDocument::FileType EParserDocument::getFileType(const QString &filePath)
{
    if (filePath.endsWith(".jpg") || filePath.endsWith(".png") || filePath.endsWith(".jpeg")) {
        return FileType::Image;
    }

    return FileType::Document;
}

QString EParserDocument::runOCRProcessByPath(QStringList imagePathList)
{
    if (imagePathList.isEmpty()) {
        qCWarning(logAIGUI) << "OCR process: Empty image path list";
        return "";
    }

    qCInfo(logAIGUI) << "Starting OCR process for images:" << imagePathList;

    // 创建 QProcess 实例
    QProcess *ocrProcess = new QProcess(this);
    
    QStringList arguments;

    // 添加图片路径参数（用逗号分隔）
    arguments << "--images" << imagePathList.join(",");

    // 添加语言参数
    arguments << "--language" << "zh-Hans_en";

    return runOCRProcessTool(ocrProcess, arguments);
}

QString EParserDocument::runOCRProcessByImage(QStringList imageList)
{
    if (imageList.isEmpty()) {
        qCWarning(logAIGUI) << "OCR process: Empty image list";
        return "";
    }

    qCInfo(logAIGUI) << "Starting OCR process for" << imageList.size() << "images";

    // 创建 QProcess 实例
    QProcess *ocrProcess = new QProcess(this);
    
    QStringList arguments;

    // 添加数据模式参数
    arguments << "--data-mode";

    // 添加语言参数
    arguments << "--language" << "zh-Hans_en";

    // 准备要发送的图片数据
    QJsonObject inputData;
    QJsonArray imageArray;

    for (int i = 0; i < imageList.size(); ++i) {
        QString base64Data = imageList[i];
        QString imageId = QString("image_%1_%2").arg(i).arg(QDateTime::currentMSecsSinceEpoch());

        QJsonObject imageObj;
        imageObj["id"] = imageId;
        imageObj["data"] = base64Data;

        imageArray.append(imageObj);
    }

    inputData["images"] = imageArray;

    // 将输入数据转换为 JSON 字符串
    QJsonDocument inputDoc(inputData);
    QByteArray inputJsonData = inputDoc.toJson(QJsonDocument::Compact);

    qCInfo(logAIGUI) << "Sending" << imageArray.size() << "images to OCR process";

    return runOCRProcessTool(ocrProcess, arguments, inputJsonData);
}

