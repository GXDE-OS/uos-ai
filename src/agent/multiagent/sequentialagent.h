#ifndef SEQUENTIALAGENT_H
#define SEQUENTIALAGENT_H

#include "llmagent.h"

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QSharedPointer>
#include <QMap>
#include <QList>

namespace uos_ai {

class SequentialAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit SequentialAgent(QObject *parent = nullptr);
    virtual ~SequentialAgent();

    /**
     * 设置模型服务
     * @param {QSharedPointer<AbstractChatModel>} llm - 模型服务的共享指针
     */
    void setModel(QSharedPointer<AbstractChatModel> llm) override;

    /**
     * 处理用户请求
     * 按子智能体的顺序依次执行子智能体的processRequest
     * @param {ModelMessage} question - 当前请求内容
     * @param {QList<ModelMessage>} messages - 聊天消息记录
     * @param {QVariantHash} params - 扩展参数
     * @returns {QVariantHash} 智能体工作流输出的消息记录
     */
    QVariantHash processRequest(const ModelMessage &question, const QList<ModelMessage> &messages, const QVariantHash &params = {}) override;
public Q_SLOTS:
    /**
     * 取消当前请求
     * 中断正在进行的LLM请求处理
     */
    void cancel() override;
protected:
    /**
     * @brief 调用子智能体前的 hook 操作
     * @param agentName 即将调用的子智能体名称
     * @param currentQuestion 当前请求内容
     * @param localMessages 局部聊天记录
     * @param globalMessages 全局聊天记录
     */
    virtual bool beforeSubAgentCall(const QString &agentName, ModelMessage &currentQuestion,
                                    QList<ModelMessage> &localMessages, const QList<ModelMessage> &globalMessages);

protected:
    QMap<QString, QSharedPointer<LlmAgent>> m_subAgents;  // 子智能体映射表
    QList<QString> m_agentOrder;  // 子智能体执行顺序
};

} // namespace uos_ai

#endif // SEQUENTIALAGENT_H 
