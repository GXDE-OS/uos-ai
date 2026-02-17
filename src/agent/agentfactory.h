#ifndef AGENTFACTORY_H
#define AGENTFACTORY_H

#include "llmagent.h"

#include <QMutex>
#include <QObject>
#include <QMap>
#include <QString>

namespace uos_ai {
class MCPServer;
typedef QSharedPointer<LlmAgent> (*CreateAgent)();
/**
 * 智能体工厂，用于创建和管理智能体
 */
class AgentFactory : public QObject
{
    Q_OBJECT
public:
    static AgentFactory *instance();

    /**
     * 注册智能体处理器
     * 
     * @param {QSharedPointer<AgentRequestHandler>} handler - 智能体请求处理器
     * @returns {bool} 是否注册成功
     */
    bool registerAgent(const QString &name, CreateAgent creator);

    /**
     * 获取智能体处理器
     * 
     * @param {QString} name - 智能体名称
     * @returns {QSharedPointer<LlmAgent>} 智能体实例，不存在则返回nullptr
     */
    QSharedPointer<LlmAgent> getAgent(const QString &name);

    /**
     * 获取智能体的MCPServer对象
     *
     * @param {QString} name - 智能体名称
     * @returns {QSharedPointer<MCPServer>} 服务对象，不存在则返回nullptr
     */
    QSharedPointer<MCPServer> getMCPServer(const QString &name);

    /**
     * 获取所有已注册的智能体名称
     * 
     * @returns {QStringList} 智能体名称列表
     */
    QStringList agentNames() const;

private:
    AgentFactory(QObject *parent = nullptr);
    ~AgentFactory();

    QMap<QString, CreateAgent> m_agents;
    mutable QMutex m_mutex;  // 添加互斥锁
};

} // namespace uos_ai

#endif // AGENTFACTORY_H 
