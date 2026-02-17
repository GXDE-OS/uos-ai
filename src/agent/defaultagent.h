#ifndef DEFAULTAGENT_H
#define DEFAULTAGENT_H

#include "mcpchatagent.h"

namespace uos_ai {

class DefaultAgent : public MCPChatAgent
{
    Q_OBJECT
public:
    explicit DefaultAgent(QObject *parent = nullptr);

    /**
     * 获取智能体的MCPServer类
     * @returns {MCPServer} MCPServer对象
     */
    QSharedPointer<MCPServer> mcpServer() const override;
    
    /**
     * 获取系统提示词
     * @returns {QString} 系统提示词内容
     */
    QString systemPrompt() const override;
    
    /**
     * 创建智能体实例的工厂方法
     * @returns {QSharedPointer<AgentRequestHandler>} 智能体请求处理器实例
     */
    static QSharedPointer<LlmAgent> create();
    
protected:
    /**
     * 初始化聊天记录
     * @param {QJsonObject} question - 当前请求内容
     * @param {QJsonArray} history - 历史消息记录
     * @returns {QJsonArray} 初始化后的消息数组
     */
    QJsonArray initChatMessages(const QJsonObject &question, const QJsonArray &history) const override;
};

}
#endif // DEFAULTAGENT_H
