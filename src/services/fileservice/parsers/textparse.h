// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TEXTPARSE_H
#define TEXTPARSE_H

#include <QByteArray>
#include <QString>

namespace uos_ai {

/**
 * @brief Plain text and source code file extraction
 *
 * Reads the file as UTF-8 text. Rejects binary files via a
 * fast heuristic check on the first 4 KB.
 */
class TextParse
{
public:
    static QString parse(const QString &filePath, QString *errorMsg = nullptr);

private:
    static bool looksBinary(const QByteArray &head);
};

} // namespace uos_ai

#endif // TEXTPARSE_H
