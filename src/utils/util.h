// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UTIL_H
#define UTIL_H

#include "uosai_global.h"

#include "serverdefs.h"

#include <QString>
#include <QPixmap>
#include <QSize>

UOSAI_BEGIN_NAMESPACE

class Util
{
public:
    static bool isWayland();
    static QString getExeNameByPid(int pid);
    static void playSystemSound_SSE_Error();
    static bool isGPTEnable();
    static bool isGPTSeries(LLMChatModel m);
    static bool isCommunity();
    static bool checkLanguage();
    static QString generateAssistantUuid(QString name);
    static bool isAIDaemonExist();
    static QString textEncodingTransferUTF8(const std::string &content);
    static bool isValidDocContent(const std::string &content);
    static bool openFileFromPath(const QString &path);
    static bool launchUosBrowser(QString url);
    static bool launchDefaultBrowser(QString url);
    static QString imageData2TmpFile(const QString &tmpDir, const QString &imageData);
    static QPixmap loadSvgPixmap(const QString &filePath, const QSize &size = QSize());
    static QString splitLocaleName(const QString &locale);
private:
    Util();
};

UOSAI_END_NAMESPACE

#endif // UTIL_H
