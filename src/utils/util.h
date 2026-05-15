// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QPixmap>
#include <QSize>

namespace uos_ai {

class Util
{
public:
    static bool isWayland();
    static QString getExeNameByPid(int pid);
    static void playSystemSound_SSE_Error();
    static bool isGPTEnable();
    static bool isCommunity();
    static bool checkLanguage();
    static bool isAIDaemonExist();
    static QString textEncodingTransferUTF8(const std::string &content);
    static bool isValidDocContent(const std::string &content);
    static bool openFileFromPath(const QString &path);
    static bool launchUosBrowser(QString url);
    static bool launchDefaultBrowser(QString url);
    static QString imageData2TmpFile(const QString &tmpDir, const QString &imageData);
    static QPixmap loadSvgPixmap(const QString &filePath, const QSize &size = QSize());
    static QString splitLocaleName(const QString &locale);
    static QString queryProcessName(const uint &pid);
private:
    Util();
};

}

#endif // UTIL_H
