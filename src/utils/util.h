// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UTIL_H
#define UTIL_H

#include "uosai_global.h"

#include <QString>

UOSAI_BEGIN_NAMESPACE

class Util
{
public:
    static bool isAccessibleEnable();
    static bool isWayland();
    static QString getExeNameByPid(int pid);
    static void playSystemSound_SSE_Error();
    static bool isGPTEnable();
    static bool isCommunity();
    static QString generateAssistantUuid(QString name);
    static bool isAIDaemonExist();

private:
    Util();
};

UOSAI_END_NAMESPACE

#endif // UTIL_H
