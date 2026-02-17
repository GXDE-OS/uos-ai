// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ALIAGENTLLM_H
#define ALIAGENTLLM_H

#include <llmmodel.h>
#include "wrapper/serverdefs.h"
#include "llm/common/networkdefs.h"

#include <QObject>

namespace uos_ai {
namespace tzdl {
struct TzdlAgentLlmConfig
{
    QString agentDisplayName;
    QString llmDisplayName;
    QString description;
    QString iconPrefix;
    QString agentIcon;
    QString llmIcon;

    TzdlAgentLlmConfig() {
        agentDisplayName = QString("智能体");
        llmDisplayName = "LLM";
        description = QString("智能体");
        iconPrefix = QString(ASSETS_INSTALL_DIR) + "icons/";
        agentIcon = "tzdl";
        llmIcon = "uoslm";
    }
};
struct TzdlConfig
{
    // 配置文件相关的成员变量
    QString serverRootUrl;
    QString agentCode;
    QString agentVersion;
    QString tokenID;
    QString createSessionRoute;
    QString runSessionRoute;
    QString clearSessionRoute;
    TzdlAgentLlmConfig agentLlmConfig;
    TzdlConfig() {
        serverRootUrl = "http://localhost:8080";
        agentCode = "e36a0ab1-b673-49d0-9534-1480a1f47c91";
        agentVersion = "170686385634";
        tokenID = "YOUR TOKEN_ID";
        createSessionRoute = "/sfm-gateway-qypcw/sfm-api-gateway/gateway/agent/api/createSession";
        runSessionRoute = "/sfm-gateway-qypcw/sfm-api-gateway/gateway/agent/api/run";
        clearSessionRoute = "/sfm-gateway-qypcw/sfm-api-gateway/gateway/agent/api/clearSession";
    }
};

class TzdlLLM : public QObject, public LLMModel
{
    Q_OBJECT
public:
    static inline QString modelID(QString &configFileName) {
        return QString("Tzdl LLM_"+configFileName);
    }

    static inline QString role(QString &configFileName) {
        return QString("tzdl_"+configFileName);
    }

public:
    // 重写构造函数，传入当前是第几个配置文件，用于生成不同的配置文件的对象
    explicit TzdlLLM(QString configFileName, TzdlConfig config, QObject *parent = nullptr);
    QString model() const override;
    QJsonObject generate(const QString &content, const QVariantHash &params, streamFuncion stream = nullptr, void *user = nullptr) override;
    void setAbort() override;

signals:
    void sigAbort();



private:
    QByteArray httpRequest(QUrl url,
                           QByteArray sendData,
                           QEventLoop &loop,
                           AIServer::ErrorType &serverErrorCode,
                           QString &errorStr,
                           QString &responseContent,
                           bool needAuth = false,
                           bool isAgentCall = false,
                           streamFuncion stream = nullptr,
                           void *user = nullptr);
    QString parseContentString(const QByteArray &content, streamFuncion stream = nullptr, void *user = nullptr);

private:
    QByteArray m_deltacontent;
    
    // 配置文件相关的成员变量
    QString m_serverRootUrl;
    QString m_agentCode;
    QString m_agentVersion;
    QString m_tokenID;
    QString m_createSessionRoute;
    QString m_runSessionRoute;
    QString m_clearSessionRoute;
    QString m_configFileName;
};

}}

#endif // ALIAGENTLLM_H
