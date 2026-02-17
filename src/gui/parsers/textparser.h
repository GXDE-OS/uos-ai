// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TEXTPARSER_H
#define TEXTPARSER_H

#include "uosai_global.h"
#include "abstractfileparser.h"

#include <QIODevice>

UOSAI_BEGIN_NAMESPACE

/**
 * @brief Text file parser
 * 
 * Parses plain text files and extracts their content.
 * Supports various text encodings including UTF-8, UTF-16, etc.
 */
class TextParser : public AbstractFileParser
{
    Q_OBJECT

public:
    explicit TextParser(QObject *parent = nullptr);
    static inline QString parserName() {
        return "default-text";
    }
    static AbstractFileParser *create();
    QString name() const override;
    QString description() const override;
    QHash<QString, float> supportedMimeTypes() const override;
    QHash<QString, float> supportedExtensions() const override;
    QSharedPointer<ParserResult> parseFile(const QString &filePath, const QVariantMap &metadata = QVariantMap()) override;

private:
    bool looksBinary(const QByteArray &head);
};

UOSAI_END_NAMESPACE

#endif // TEXTPARSER_H
