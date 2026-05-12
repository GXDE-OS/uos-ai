// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "textparse.h"

#include <QCoreApplication>
#include <QFile>
#include <QLoggingCategory>

#include <climits>

Q_DECLARE_LOGGING_CATEGORY(logService)

using namespace uos_ai;

QString TextParse::parse(const QString &filePath, QString *errorMsg)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMsg)
            *errorMsg = QCoreApplication::translate("TextParse", "Unable to open file: %1")
                            .arg(file.errorString());
        return {};
    }

    // Read the first 4 KB to detect binary content before loading the whole file.
    const QByteArray head = file.read(4096);
    if (looksBinary(head)) {
        if (errorMsg)
            *errorMsg = QCoreApplication::translate("TextParse", "The file content is in binary format");
        return {};
    }

    // Seek back and stream the file in chunks to avoid a 2× memory peak
    // that would occur if we did readAll() + QString::fromUtf8(data) separately.
    if (!file.seek(0)) {
        if (errorMsg)
            *errorMsg = QCoreApplication::translate("TextParse", "Unable to read file: %1")
                            .arg(file.errorString());
        return {};
    }

    QString result;
    result.reserve(static_cast<int>(qMin(file.size(), qint64(INT_MAX))));
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(65536);
        result += QString::fromUtf8(chunk);
    }
    file.close();
    return result;
}

// Detect binary by scanning the first 4 KB for null bytes or non-printable
// control characters (excluding tab / LF / CR).
bool TextParse::looksBinary(const QByteArray &head)
{
    const char *p   = head.constData();
    const int  limit = qMin(4096, head.size());
    for (int i = 0; i < limit; ++i) {
        const unsigned char c = static_cast<unsigned char>(p[i]);
        if (c == '\0' || (c < 0x20 && c != '\t' && c != '\n' && c != '\r'))
            return true;
    }
    return false;
}
