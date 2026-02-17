#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFileInfo>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QBuffer>
#include <QImage>
#include <QTemporaryFile>
#include <DOcr>
#include <QLoggingCategory>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using namespace Qt;
#endif

Q_LOGGING_CATEGORY(logOCR, "uosai.ocr")

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("uos-ai-ocr-process");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("UOS AI OCR Process");
    parser.addHelpOption();
    parser.addVersionOption();

    // 添加图片路径参数
    QCommandLineOption imagePathsOption(QStringList() << "i" << "images",
                                       "Image file paths to process (comma separated)",
                                       "paths");
    parser.addOption(imagePathsOption);

    // 添加图片数据模式参数
    QCommandLineOption imageDataModeOption(QStringList() << "d" << "data-mode",
                                          "Process image data from stdin instead of file paths");
    parser.addOption(imageDataModeOption);

    // 添加语言选项
    QCommandLineOption languageOption(QStringList() << "l" << "language",
                                     "OCR language (default: zh-Hans_en)",
                                     "language", "zh-Hans_en");
    parser.addOption(languageOption);

    parser.process(app);

    QString language = parser.value(languageOption);
    bool dataMode = parser.isSet(imageDataModeOption);

    QTextStream out(stdout);
    QTextStream err(stderr);

    // 创建 DOcr 实例
    Dtk::Ocr::DOcr ocr;

    // 尝试用v5
    auto plugins = ocr.installedPluginNames();
    const QString ocrv5 = "PPOCR_V5";
    bool load = false;
    if (plugins.contains(ocrv5)) {
        if (ocr.loadPlugin(ocrv5)) {
            load = true;
        } else {
            qCWarning(logOCR) << "Fail to load OCR V5 plugin" << '\n';
        }
    }

    // 加载默认插件
    if (!load && !ocr.loadDefaultPlugin()) {
        qCWarning(logOCR) << "Error: Failed to load default OCR plugin" << '\n';
        return 1;
    }

    // 设置使用硬件加速
    ocr.setUseHardware({{Dtk::Ocr::GPU_Vulkan, 0}});

    // 设置识别语言
    ocr.setLanguage(language);

    QJsonArray results;
    int totalImages = 0;
    int processedImages = 0;

    if (dataMode) {
        // 数据模式：从标准输入读取图片数据
        QTextStream in(stdin);
        QString input = in.readAll();
        
        QJsonParseError parseError;
        QJsonDocument inputDoc = QJsonDocument::fromJson(input.toUtf8(), &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            qCCritical(logOCR) << "Error: Failed to parse input JSON: " << parseError.errorString();
            return 1;
        }
        
        if (!inputDoc.isObject()) {
            qCCritical(logOCR) << "Error: Input JSON must be an object";
            return 1;
        }
        
        QJsonObject inputObj = inputDoc.object();
        QJsonArray imageDataArray = inputObj["images"].toArray();
        totalImages = imageDataArray.size();
        
        // 处理每个图片数据
        for (int i = 0; i < imageDataArray.size(); ++i) {
            QJsonObject imageDataObj = imageDataArray[i].toObject();
            QString imageId = imageDataObj["id"].toString();
            QString imageDataStr = imageDataObj["data"].toString();
            
            // 解码 base64 图片数据
            QByteArray imageData = QByteArray::fromBase64(imageDataStr.toUtf8());
            
            QJsonObject imageResult;
            imageResult["imageId"] = imageId;
            imageResult["index"] = i;
            
            // 从内存中创建 QImage
            QImage image;
            if (!image.loadFromData(imageData)) {
                imageResult["success"] = false;
                imageResult["error"] = "Invalid image data";
                imageResult["result"] = "";
                
                // 输出单个图片的错误结果
                QJsonObject singleResult;
                singleResult["type"] = "single_result";
                singleResult["data"] = imageResult;
                out << QJsonDocument(singleResult).toJson(QJsonDocument::Compact) << '\n';
                out.flush();
                
                results.append(imageResult);
                processedImages++;
                continue;
            }
            
            // 设置 QImage 对象到 OCR
            ocr.setImage(image);
            
            // 执行 OCR 分析
            bool analyzeSuccess = ocr.analyze();
            
            imageResult["success"] = analyzeSuccess;
            
            if (analyzeSuccess) {
                QString ocrResult = ocr.simpleResult();
                imageResult["result"] = ocrResult;
                imageResult["error"] = "";
            } else {
                imageResult["result"] = "";
                imageResult["error"] = "OCR analyze failed";
            }
            
            // 输出单个图片的结果
            QJsonObject singleResult;
            singleResult["type"] = "single_result";
            singleResult["data"] = imageResult;
            out << QJsonDocument(singleResult).toJson(QJsonDocument::Compact) << '\n';
            out.flush();
            
            results.append(imageResult);
            processedImages++;
            
            // 输出进度信息
            QJsonObject progressInfo;
            progressInfo["type"] = "progress";
            progressInfo["processed"] = processedImages;
            progressInfo["total"] = totalImages;
            progressInfo["percentage"] = (processedImages * 100) / totalImages;
            out << QJsonDocument(progressInfo).toJson(QJsonDocument::Compact) << '\n';
            out.flush();
        }
        
    } else {
        // 文件路径模式：处理文件路径
        if (!parser.isSet(imagePathsOption)) {
            qCCritical(logOCR) << "Error: Image paths are required in file mode";
            parser.showHelp(1);
            return 1;
        }
        
        QString imagePathsStr = parser.value(imagePathsOption);
        QStringList imagePaths = imagePathsStr.split(",");
        
        if (imagePaths.isEmpty()) {
            qCCritical(logOCR) << "Error: No valid image paths provided";
            return 1;
        }
        
        totalImages = imagePaths.size();
        
        // 处理每个图片
        for (const QString &imagePath : imagePaths) {
            QString trimmedPath = imagePath.trimmed();
            
            // 检查文件是否存在
            if (!QFileInfo::exists(trimmedPath)) {
                qWarning() << "Warning: Image file does not exist: " << trimmedPath << '\n';
                
                // 添加错误结果
                QJsonObject errorResult;
                errorResult["imagePath"] = trimmedPath;
                errorResult["success"] = false;
                errorResult["error"] = "File does not exist";
                errorResult["result"] = "";
                results.append(errorResult);
                
                processedImages++;
                continue;
            }

            // 设置图片文件
            ocr.setImageFile(trimmedPath);

            // 执行 OCR 分析
            bool analyzeSuccess = ocr.analyze();

            QJsonObject imageResult;
            imageResult["imagePath"] = trimmedPath;
            imageResult["success"] = analyzeSuccess;

            if (analyzeSuccess) {
                QString ocrResult = ocr.simpleResult();
                imageResult["result"] = ocrResult;
                imageResult["error"] = "";
                
                // 输出单个图片的结果（实时反馈）
                QJsonObject singleResult;
                singleResult["type"] = "single_result";
                singleResult["data"] = imageResult;
                out << QJsonDocument(singleResult).toJson(QJsonDocument::Compact) << '\n';
                out.flush();
                
            } else {
                imageResult["result"] = "";
                imageResult["error"] = "OCR analyze failed";
                
                // 输出单个图片的错误结果
                QJsonObject singleResult;
                singleResult["type"] = "single_result";
                singleResult["data"] = imageResult;
                out << QJsonDocument(singleResult).toJson(QJsonDocument::Compact) << '\n';
                out.flush();
            }

            results.append(imageResult);
            processedImages++;

            // 输出进度信息
            QJsonObject progressInfo;
            progressInfo["type"] = "progress";
            progressInfo["processed"] = processedImages;
            progressInfo["total"] = totalImages;
            progressInfo["percentage"] = (processedImages * 100) / totalImages;
            out << QJsonDocument(progressInfo).toJson(QJsonDocument::Compact) << '\n';
            out.flush();
        }
    }

    // 输出全部结果
//    QJsonObject finalResult;
//    finalResult["type"] = "final_result";
//    finalResult["totalImages"] = totalImages;
//    finalResult["processedImages"] = processedImages;
//    finalResult["results"] = results;

//    out << QJsonDocument(finalResult).toJson(QJsonDocument::Compact) << endl;
//    out.flush();

    return 0;
} 
