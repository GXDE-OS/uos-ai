// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LLMSERVICEVENDOR_H
#define LLMSERVICEVENDOR_H

#include "serverdefs.h"
#include "externalllm/externalllmloader.h"
#include "externalllm/externalagentloader.h"
#include "externalllm/externalagent.h"
#include "externalllm/modelhubllm.h"
#include "llmplugin.h"
#include "llm.h"

#include <QObject>
#include <QMutex>
#include <QMutexLocker>

#define LLMVendor  uos_ai::LLMServiceVendor::instance

namespace uos_ai {

class LLMServiceVendor : public QObject
{
    Q_OBJECT
public:
    explicit LLMServiceVendor(QObject *parent = nullptr);

    // 单例模式 - 获取全局唯一实例
    static LLMServiceVendor *instance();

    void initExternal();
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
    QString queryIconById(const QString &assistantId, const QString &modelId);
    QString queryDisplayNameById(const QString &assistantId);

    QVariant getFAQ(const QString &assistantId);
    
protected:
    void initModelPlugin();
    void initAgent();
    
signals:
    void agentChanged();
    
public slots:
    
private slots:
    void updateAgent();

private:  
    // 成员变量互斥锁
    mutable QMutex m_mutex;
    
    QMap<QString, LLMServerProxy> extServer;
    QMap<QString, QSharedPointer<LLMPlugin>> extPlugins;
    uos_ai::ExternalLLMLoader llmLoader;
    uos_ai::ExternalAgentLoader agentLoader;
    ModelHubLLM modelhub;
    bool inited = false;

};

}

#endif // LLMSERVICEVENDOR_H
