// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef OCRPARSE_H
#define OCRPARSE_H

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace uos_ai {

/**
 * @brief OCR-based image text extraction
 *
 * Parses jpg/jpeg/png files via the uos-ai-ocr-process subprocess.
 * Must be called from a background thread (blocking).
 */
class OcrParse
{
public:
    static QString parse(const QString &filePath, QString *errorMsg = nullptr);
    static QString parseBase64Images(const QStringList &base64Images);
    static bool    isAvailable();

private:
    static QString runOCRProcess(const QStringList &arguments,
                                 const QByteArray  &inputData = QByteArray());

    static constexpr const char *OCR_BINARY =
        "/usr/lib/uos-ai-assistant/uos-ai-ocr-process";
};

} // namespace uos_ai

#endif // OCRPARSE_H
