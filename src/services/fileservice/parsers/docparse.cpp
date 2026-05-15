// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "docparse.h"

#include "utils/util.h"

#include <QCoreApplication>
#include <QLoggingCategory>

#include <docparser.h>

Q_DECLARE_LOGGING_CATEGORY(logService)

using namespace uos_ai;

QString DocParse::parse(const QString &filePath, QString *errorMsg)
{
    std::string stdContent = DocParser::convertFile(filePath.toStdString());

    if (!Util::isValidDocContent(stdContent)) {
        if (errorMsg)
            *errorMsg = QCoreApplication::translate("DocParse", "The document content is invalid or empty.");
        return {};
    }

    return Util::textEncodingTransferUTF8(stdContent);
}
