// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EXTERALAGENTLOADER_H
#define EXTERALAGENTLOADER_H

#include "externalagent.h"
#include "utils/globalfilewatcher.h"

#include <QObject>
#include <QSharedPointer>
#include <QTimer>

namespace uos_ai {

class ExternalAgentLoader : public QObject
{
    Q_OBJECT
public:
    explicit ExternalAgentLoader(QObject *parent = nullptr);
    ~ExternalAgentLoader();
    void setPaths(const QStringList &paths);
    void readAgents();
    QList<QSharedPointer<ExternalAgent>> agents() const;
signals:
    void agentChanged();
public slots:
private:
    QStringList loadPaths;
    QList<QSharedPointer<ExternalAgent> > allAgents;
    FileWatcher watcher;
    QTimer agentTimer;
};
}

#endif // EXTERALAGENTLOADER_H
