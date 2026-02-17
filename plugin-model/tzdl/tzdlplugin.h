// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TZDLPLUGIN_H
#define TZDLPLUGIN_H

#include <llmplugin.h>
#include "tzdlllm.h"

#include <QObject>

namespace uos_ai {
namespace tzdl {
class TzdlPlugin : public QObject, public LLMPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LLMPlugin_Meta_IID FILE "plugin.json")
    Q_INTERFACES(uos_ai::LLMPlugin)
public:
    explicit TzdlPlugin(QObject *parent = nullptr);
    QStringList modelList() const override;
    QStringList roles(const QString &model) const override;
    QVariant queryInfo(const QString &query, const QString &id) override;
    LLMModel *createModel(const QString &name) override;
protected:
    void loadFAQ();
private:
    void initConfigFile();  //初始化配置文件
    // 配置文件读写方法
    void writeConfigToFile();
    void readConfigFromFile();
signals:

public slots:
private:
    QList<QJsonObject> m_faq;
    int m_configFileCount = 0;  // 配置文件数量
    QStringList m_configFileLists = {};  // 配置文件列表
    QMap<QString, TzdlConfig> m_config;  //配置文件及对应的配置项
    QString m_baseConfigPath; // 配置文件基础路径
};
}}

#endif // TZDLPLUGIN_H
