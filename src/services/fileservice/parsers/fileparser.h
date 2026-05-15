// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILEPARSER_H
#define FILEPARSER_H

#include <QString>
#include <QStringList>

namespace uos_ai {

/**
 * @brief Unified file text extraction interface
 *
 * Routes a file path to one of three parsing strategies based on
 * its extension, then returns the extracted plain text.
 *
 * All methods are static — no instance is required.
 *
 * Usage:
 *   QString errorMsg;
 *   QString text = FileParser::parse(filePath, &errorMsg);
 */
class FileParser
{
public:
    enum class Strategy {
        Doc,         ///< PDF / Office / ODF  → DocParse (docparser library)
        Text,        ///< Plain text / source  → TextParse (direct UTF-8 read)
        Ocr,         ///< Image (jpg/jpeg/png) → OcrParse (OCR subprocess)
        Unsupported  ///< File type not handled
    };

    /**
     * @brief Determine parsing strategy from file extension (O(1) lookup).
     */
    static Strategy strategyFor(const QString &filePath);

    /**
     * @brief Extract text from a file.
     * @param filePath  Absolute path to the file.
     * @param errorMsg  Human-readable error message on failure (may be nullptr).
     * @return Extracted text, or empty string on failure.
     */
    static QString parse(const QString &filePath, QString *errorMsg = nullptr);

    /**
     * @brief OCR for base64-encoded images (screenshots / clipboard).
     * @param base64Images List of base64-encoded PNG/JPEG strings.
     * @return Recognised text, or empty string on failure.
     */
    static QString parseBase64Images(const QStringList &base64Images);

    /**
     * @brief Supported file suffixes with "*." prefix, for file dialog filtering.
     */
    static const QStringList &supportedSuffixes();
};

} // namespace uos_ai

#endif // FILEPARSER_H
