// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZTBPLUGIN_H
#define ZTBPLUGIN_H

#include <llmplugin.h>

#include <QObject>

namespace uos_ai {
namespace ztb {
class ZtbPlugin : public QObject, public LLMPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LLMPlugin_Meta_IID FILE "plugin.json")
    Q_INTERFACES(uos_ai::LLMPlugin)
public:
    explicit ZtbPlugin(QObject *parent = nullptr);
    QStringList modelList() const override;
    QStringList roles(const QString &model) const override;
    QVariant queryInfo(const QString &query, const QString &id) override;
    LLMModel *createModel(const QString &name) override;
protected:
    void loadFAQ();
signals:

public slots:
private:
    QList<QJsonObject> m_faq;
};
}}

#endif // ZTBPLUGIN_H
