#ifndef MODELHUBWRAPPER_H
#define MODELHUBWRAPPER_H
// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QObject>
#include <QReadWriteLock>

namespace uos_ai {

class ModelhubWrapper : public QObject
{
    Q_OBJECT
public:
    explicit ModelhubWrapper(const QString &model, QObject *parent = nullptr);
    ~ModelhubWrapper();
    bool isRunning();
    bool ensureRunning();
    bool health();
    void setKeepLive(bool live);
    QString urlPath(const QString &api) const;
    static bool isModelhubInstalled();
    static bool isModelInstalled(const QString &model);
    static QVariantHash modelStatus(const QString &model);
    static QList<QVariantHash> modelsStatus();
    static bool openCmd(const QString &cmd, QString &out);
    static QStringList installedModels();
protected:
    void updateHost();
protected:
    bool keepLive = false;
    QString modelName;
    QString host;
    int port = -1;
    qint64 pid = -1;
    mutable QReadWriteLock lock;
};
}

#endif   // MODELHUBWRAPPER_H
