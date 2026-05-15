// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DOCPARSE_H
#define DOCPARSE_H

#include <QString>

namespace uos_ai {

/**
 * @brief Document text extraction via the docparser library
 *
 * Handles PDF, Word (doc/docx), Excel (xls/xlsx),
 * PowerPoint (ppt/pptx), ODF (odt/ods/odp), and WPS formats.
 */
class DocParse
{
public:
    static QString parse(const QString &filePath, QString *errorMsg = nullptr);
};

} // namespace uos_ai

#endif // DOCPARSE_H
