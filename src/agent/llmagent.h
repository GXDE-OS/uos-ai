#ifndef LLMAGENT_H
#define LLMAGENT_H

#include "agent.h"
#include "llm.h"

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QSharedPointer>

namespace uos_ai {

class LlmAgent : public QObject, public Agent
{
    Q_OBJECT
public:
    struct OutputCtx {
        OutputCtx(){}
        virtual ~OutputCtx(){}
        QString deltaContent; //模型输出的当前内容块
    };
public:
    explicit LlmAgent(QObject *parent = nullptr);

    /**
     * 初始化智能体
     * @returns {bool} 是否成功
     */
    virtual bool initialize();

    /**
     * 设置模型服务
     * @param {QSharedPointer<LLM>} llm - 模型服务的共享指针
     */
    virtual void setModel(QSharedPointer<LLM> llm);

    /**
     * 处理用户请求
     * 处理来自用户的请求，包括文本对话和工具调用
     * @param {QJsonObject} question - 当前请求内容，包含用户的问题和相关参数
     * @param {QJsonArray} history - 聊天消息记录，包含之前的对话历史
     * @param {QJsonObject} params - 扩展参数，包含额外的配置信息
     * @returns {QJsonObject} 智能体工作流输出的消息记录
     */
    QJsonObject processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params = {}) override;

    virtual int lastError() const;
    virtual QString lastErrorString() const;

Q_SIGNALS:
    /**
     * 当有新的聊天增量内容可读时发出此信号
     * @param {QString} deltaData - 增量数据内容，包含新的文本或工具调用结果
     */
    void readyReadChatDeltaContent(const QString &deltaData);

public Q_SLOTS:
    /**
     * 取消当前请求
     * 中断正在进行的LLM请求处理
     */
    virtual void cancel();

protected:
    /**
     * 初始化聊天记录
     * 将用户请求和历史记录整合成完整的聊天消息数组
     * @param {QJsonObject} question - 当前请求内容
     * @param {QJsonArray} history - 历史消息记录
     * @returns {QJsonArray} 初始化后的消息数组，包含系统提示、历史记录和当前请求
     */
    virtual QJsonArray initChatMessages(const QJsonObject &question, const QJsonArray &history) const;

    /**
     * 处理模型流式输出的内容
     * 解析模型的流式输出，识别文本内容和工具调用
     * @param {OutputCtx *} ctx - 上下文
     * @returns {bool} 处理是否成功，true表示成功处理，false表示处理失败
     */
    virtual bool handleStreamOutput(OutputCtx *ctx);

    /**
     * 调用工具
     * 执行指定的工具调用，传递相应的参数
     * @param {QString} toolName - 要调用的工具名称
     * @param {QJsonObject} params - 工具调用所需的参数
     * @returns {QPair<int, QString>} 工具调用的结果，包含状态码和返回内容
     *          状态码：0表示成功，非0表示错误
     *          QString：工具调用的返回内容或错误信息
     */
    virtual QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params);

    virtual void textChainContent(const QString &content);
protected:
    bool canceled = false; // 是否取消
    QSharedPointer<LLM> m_llm; // 模型服务
};

} // namespace uos_ai

#endif // LLMAGENT_H 
