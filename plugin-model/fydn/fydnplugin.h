// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FYDNPLUGIN_H
#define FYDNPLUGIN_H

#include <llmplugin.h>

#include <QObject>
#include <QVector>

namespace uos_ai {
namespace fydn {

class FydnPlugin : public QObject, public LLMPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LLMPlugin_Meta_IID FILE "plugin.json")
    Q_INTERFACES(uos_ai::LLMPlugin)
public:
    explicit FydnPlugin(QObject *parent = nullptr);
    QStringList modelList() const override;
    QStringList roles(const QString &model) const override;
    QVariant queryInfo(const QString &query, const QString &id) override;
    LLMModel *createModel(const QString &name) override;

    void loadFAQ();

signals:

public slots:

private:
    QVector<QJsonObject> m_regulatoryInquiryFAQ;
    QVector<QJsonObject> m_caseInquiryFAQ;
};

}}
#endif // FYDNPLUGIN_H
