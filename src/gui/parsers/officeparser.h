// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef OFFICEPARSER_H
#define OFFICEPARSER_H

#include "uosai_global.h"
#include "abstractfileparser.h"

UOSAI_BEGIN_NAMESPACE

/**
 * @brief Office document parser
 * 
 * Parses Microsoft Office documents (Word, Excel, PowerPoint)
 * and extracts text content using external tools.
 */
class OfficeParser : public AbstractFileParser
{
    Q_OBJECT

public:
    explicit OfficeParser(QObject *parent = nullptr);
    static inline QString parserName() {
        return "default-office";
    }
    static AbstractFileParser *create();
    QString name() const override;
    QString description() const override;
    QHash<QString, float> supportedMimeTypes() const override;
    QHash<QString, float> supportedExtensions() const override;
    QSharedPointer<ParserResult> parseFile(const QString &filePath, const QVariantMap &metadata = QVariantMap()) override;
};

UOSAI_END_NAMESPACE

#endif // OFFICEPARSER_H
