// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LLMSERVICEVENDOR_H
#define LLMSERVICEVENDOR_H

#include "serverdefs.h"
#include "externalllm/exteralllmloader.h"
#include "llmplugin.h"
#include "llm.h"

#include <QObject>

namespace uos_ai {

class LLMServiceVendor : public QObject
{
    Q_OBJECT
public:
    explicit LLMServiceVendor(QObject *parent = nullptr);
    void initModelPlugin();
    LLMServerProxy checkUpdateLLmAccount(const QString &llmId);
    LLMServerProxy queryValidServerAccount(const QString &llmId);
    QList<LLMServerProxy> queryServerAccountByRole(const AssistantProxy &role);

    LLMServerProxy setCurrentLLMAccountId(const QString &id, const QString &app);
    QString queryLLMAccountList(const QList<LLMChatModel> &excludes);
    QString queryLLMAccountListByRole(const AssistantProxy &role, const QList<LLMChatModel> &excludes);
    QString queryCurLlmIdByAppId(const QString &appid);
    QSharedPointer<LLM> getCopilot(const LLMServerProxy &serverproxy);

    QList<AssistantProxy> queryAssistantList();
    AssistantProxy queryAssistantById(const QString &assistantId);

    QVariant getFAQ(const QString &assistantId);

signals:

public slots:

private:
    QMap<QString, LLMServerProxy> extServer;
    QMap<QString, QSharedPointer<LLMPlugin>> extPlugins;
    uos_ai::ExteralLLMLoader llmLoader;
};

}
#endif // LLMSERVICEVENDOR_H
