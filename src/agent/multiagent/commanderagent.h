#ifndef COMMANDERAGENT_H
#define COMMANDERAGENT_H

#include "llmagent.h"

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QSharedPointer>
#include <QMap>
#include <QList>

namespace uos_ai {

class CommanderAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit CommanderAgent(QObject *parent = nullptr);
    virtual ~CommanderAgent();

    /**
     * 初始化智能体
     * @returns {bool} 是否成功
     */
    bool initialize() override;

    /**
     * @brief 获取系统提示词
     * @return 包含子智能体信息的系统提示词
     */
    QString systemPrompt() const override;

    /**
     * 设置模型服务
     * @param {QSharedPointer<LLM>} llm - 模型服务的共享指针
     */
    void setModel(QSharedPointer<LLM> llm) override;
protected:
    /**
     * @brief 初始化聊天消息
     * @param question 当前请求内容
     * @param messages 历史消息记录
     * @return 初始化后的消息数组
     */
    QJsonArray initChatMessages(const QJsonObject &question, const QJsonArray &messages) const override;

    /**
     * @brief 调用工具
     * @param toolName 工具名称
     * @param params 工具参数
     * @return 工具调用结果
     */
    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;

    /**
     * @brief 构建子智能体工具定义
     * @return 工具定义JSON数组
     */
    virtual QJsonObject subagentTool() const;

    /**
     * @brief 构建子智能体描述文本
     * @return 子智能体描述文本
     */
    virtual QString subagentPrompt() const;

protected:
    QMap<QString, QSharedPointer<LlmAgent>> m_subAgents;  // 子智能体映射表
};

} // namespace uos_ai

#endif // COMMANDERAGENT_H 
