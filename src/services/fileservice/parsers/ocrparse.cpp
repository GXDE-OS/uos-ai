// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ocrparse.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QProcess>

Q_DECLARE_LOGGING_CATEGORY(logService)

using namespace uos_ai;

QString OcrParse::parse(const QString &filePath, QString *errorMsg)
{
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        if (errorMsg)
            *errorMsg = QCoreApplication::translate("OcrParse", "The image file does not exist: %1").arg(filePath);
        return {};
    }

    const QStringList arguments = { "--images", filePath, "--language", "zh-Hans_en" };
    const QString text = runOCRProcess(arguments);

    if (text.isEmpty()) {
        if (errorMsg)
            *errorMsg = QCoreApplication::translate("OcrParse", "OCR did not recognize the text content");
        return {};
    }

    return text;
}

QString OcrParse::parseBase64Images(const QStringList &base64Images)
{
    if (base64Images.isEmpty())
        return {};

    const qint64 ts = QDateTime::currentMSecsSinceEpoch();
    QJsonArray imageArray;
    for (int i = 0; i < base64Images.size(); ++i) {
        QJsonObject obj;
        obj["id"]   = QString("image_%1_%2").arg(i).arg(ts);
        obj["data"] = base64Images[i];
        imageArray.append(obj);
    }
    QJsonObject root;
    root["images"] = imageArray;

    const QByteArray inputJson = QJsonDocument(root).toJson(QJsonDocument::Compact);
    const QStringList arguments = { "--data-mode", "--language", "zh-Hans_en" };
    return runOCRProcess(arguments, inputJson);
}

bool OcrParse::isAvailable()
{
    return QFileInfo(OCR_BINARY).isExecutable();
}

QString OcrParse::runOCRProcess(const QStringList &arguments, const QByteArray &inputData)
{
    qCInfo(logService) << "Starting OCR process:" << OCR_BINARY << arguments;

    QProcess proc;
    proc.start(OCR_BINARY, arguments);

    if (!proc.waitForStarted(5000)) {
        qCWarning(logService) << "OCR process failed to start:" << proc.errorString();
        return {};
    }

    if (!inputData.isEmpty()) {
        proc.write(inputData);
        proc.closeWriteChannel();
    }

    if (!proc.waitForFinished(900000)) {  // 15-minute timeout
        qCWarning(logService) << "OCR process timed out";
        proc.kill();
        return {};
    }

    qCInfo(logService) << "OCR process finished, exit code:" << proc.exitCode();

    const QString stderrOut = QString::fromUtf8(proc.readAllStandardError()).trimmed();
    if (!stderrOut.isEmpty()
        && !stderrOut.contains("QML")
        && !stderrOut.contains("libpng warning")
        && !stderrOut.contains("deprecated")) {
        qCWarning(logService) << "OCR stderr:" << stderrOut;
    }

    QString allResults;
    const QStringList lines = QString::fromUtf8(proc.readAllStandardOutput()).split('\n');
    for (const QString &line : lines) {
        if (line.isEmpty())
            continue;

        QJsonParseError jsonErr;
        const QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &jsonErr);
        if (jsonErr.error != QJsonParseError::NoError) {
            qCWarning(logService) << "JSON parse error:" << jsonErr.errorString();
            continue;
        }

        const QJsonObject obj  = doc.object();
        const QString     type = obj["type"].toString();

        if (type == "single_result") {
            const QJsonObject data    = obj["data"].toObject();
            const bool        success = data["success"].toBool();
            const QString     text    = data["result"].toString();
            if (success) {
                if (!allResults.isEmpty())
                    allResults += "\n\n";
                allResults += text;
                qCInfo(logService) << "OCR result:" << text.left(80);
            } else {
                qCWarning(logService) << "OCR error:" << data["error"].toString();
            }
        } else if (type == "progress") {
            qCInfo(logService) << "OCR progress:"
                                << obj["processed"].toInt() << "/"
                                << obj["total"].toInt()
                                << "(" << obj["percentage"].toInt() << "%)";
        }
    }

    return allResults;
}
