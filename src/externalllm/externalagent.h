// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EXTERNALAGENT_H
#define EXTERNALAGENT_H

#include "serverdefs.h"
#include "llmplugin.h"

#include <QSharedPointer>

namespace uos_ai {

class ExternalAgent : public  LLMPlugin
{
public:
    explicit ExternalAgent();
    static QSharedPointer<ExternalAgent> fromJson(const QVariantHash &root);
    static LLMChatModel modelType(const QString &strType);
public:
    QStringList roles(const QString &model) const override;
    QVariant queryInfo(const QString &query, const QString &id) override;
    QStringList modelList() const override;
    LLMModel *createModel(const QString &name) override;
    inline QList<LLMServerProxy> getModels() const  {
        return models.values();
    }
protected:
    QString agentId;
    QString name;
    QString description;
    QString iconName;
    QString prefix;
    QString faqFile;
    QMap<QString, LLMServerProxy> models;
    QMap<QString, QString> modelIcon;
};

}
#endif // EXTERNALAGENT_H
