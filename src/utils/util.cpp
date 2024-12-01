// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "util.h"

#include <DDesktopServices>
#include <DWidget>
#include <DtkGuis>
#include <DSysInfo>

#include <QGSettings>
#include <QVariant>
#include <QProcessEnvironment>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
UOSAI_USE_NAMESPACE

Util::Util()
{

}

bool Util::isAccessibleEnable()
{
    static QByteArray ifs = "org.gnome.desktop.interface";
    if (!QGSettings::isSchemaInstalled(ifs))
        return false;

    QGSettings gs(ifs);
    return gs.get("toolkitAccessibility").toBool();
}

bool Util::isWayland()
{
    static int wayland = -1;
    if (wayland < 0) {
        auto e = QProcessEnvironment::systemEnvironment();
        QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
        QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));
        if (XDG_SESSION_TYPE == QLatin1String("wayland") || WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)) {
            wayland = 1;
        } else {
            wayland = 0;
        }
        qInfo() << "is wayland" << wayland;
    }

    return wayland == 1;
}

QString Util::getExeNameByPid(int pid)
{
    QString exeName;
    QString linkpath = QString("/proc/%1/exe").arg(pid);
    QString path = QFile::symLinkTarget(linkpath);
    auto args = path.split("/");
    exeName = args.length() == 0 ? "" : args[args.length() -1];
    if (exeName.length() == 0) {
        //如：/usr/bin/xxxx -e xx
        QString cmdline;
        QFile fileCmd(QString("/proc/%1/cmdline").arg(pid));
        if (fileCmd.open(QFile::ReadOnly)) {
            cmdline = QString(fileCmd.readLine());
            fileCmd.close();
        }
        QStringList list = cmdline.split("/");
        QString program = list.last();
        list = program.split(" ");
        exeName = list.first();
    }
    return exeName;
}

void Util::playSystemSound_SSE_Error()
{
    DDesktopServices::playSystemSoundEffect(
                DDesktopServices::SystemSoundEffect::SSE_Error);
}

bool Util::isGPTEnable()
{
    qDebug() << "productType" << DSysInfo::productType()
             << "language" << QLocale::system().language() << QLocale::system().script();
    return (DSysInfo::productType() == DSysInfo::Deepin) && (QLocale::English == QLocale::system().language());
}

bool Util::isCommunity()
{
//    return true;
    return DSysInfo::productType() == DSysInfo::Deepin;
}

QString Util::generateAssistantUuid(QString name)
{
    QString formatName = name.replace(" ", "-").toLower().trimmed();
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    QString uniqueId = formatName + "_" + QString::number(timestamp);
    return  uniqueId;
}

bool Util::isAIDaemonExist()
{
    QDir dir("/usr/bin");
    if (!dir.exists()) {
        return false;
    }

    QFileInfo fileInfo(dir.filePath("deepin-ai-daemon"));
    return fileInfo.exists() && fileInfo.isFile();
}
