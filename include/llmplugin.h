// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LLMPLUGIN_H
#define LLMPLUGIN_H

#include <QObject>

#define LLMPlugin_Meta_IID "org.deepin.plugin.uos-ai.llmplugin"

namespace uos_ai {

#define QUERY_DISPLAY_NAME "displayname"
#define QUERY_ICON_NAME "iconname"
#define QUERY_ICON_PREFIX "iconprefix"
#define QUERY_DESCRIPTION  "description"
#define QUERY_QUESTIONS  "questions"

class LLMModel;
class LLMPlugin
{
public:
    explicit LLMPlugin() {}
    virtual ~LLMPlugin() {}
    virtual QStringList roles(const QString &model) const = 0;
    virtual QVariant queryInfo(const QString &query, const QString &id) = 0;
    virtual QStringList modelList() const = 0;
    virtual LLMModel *createModel(const QString &name) = 0;
};

}

Q_DECLARE_INTERFACE(uos_ai::LLMPlugin, LLMPlugin_Meta_IID)

#endif // LLMPLUGIN_H
