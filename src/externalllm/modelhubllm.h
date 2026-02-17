// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MODELHUBLLM_H
#define MODELHUBLLM_H

#include "serverdefs.h"

#include <QObject>
#include <QMutex>

namespace uos_ai {
class ModelhubWrapper;
class ModelHubLLM : public QObject
{
    Q_OBJECT
public:
    explicit ModelHubLLM(QObject *parent = nullptr);
    QList<LLMServerProxy> modelList();
    QSharedPointer<ModelhubWrapper> getWrapper(const QString &id);
signals:

public slots:
private:
    QMutex mtx;
    QMap<QString, QSharedPointer<ModelhubWrapper>> wrapper;
};

}
#endif // MODELHUBLLM_H
