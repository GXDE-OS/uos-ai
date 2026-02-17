#ifndef APPAGENT_H
#define APPAGENT_H

#include "mcpchatagent.h"

namespace uos_ai {

class AppAgent : public MCPChatAgent
{
    Q_OBJECT
public:
    explicit AppAgent(QObject *parent = nullptr);
    
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
#endif // APPAGENT_H
