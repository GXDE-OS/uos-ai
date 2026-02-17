// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PDFPARSER_H
#define PDFPARSER_H

#include "uosai_global.h"
#include "abstractfileparser.h"

UOSAI_BEGIN_NAMESPACE

/**
 * @brief PDF file parser
 * 
 * Parses PDF files and extracts text content.
 * Uses external PDF processing libraries for text extraction.
 */
class PdfParser : public AbstractFileParser
{
    Q_OBJECT

public:
    explicit PdfParser(QObject *parent = nullptr);
    static AbstractFileParser *create();
    static inline QString parserName() {
        return "default-pdf";
    }
    QString name() const override;
    QString description() const override;
    QHash<QString, float> supportedMimeTypes() const override;
    QHash<QString, float> supportedExtensions() const override;
    QSharedPointer<ParserResult> parseFile(const QString &filePath, const QVariantMap &metadata = QVariantMap()) override;
};

UOSAI_END_NAMESPACE

#endif // PDFPARSER_H
