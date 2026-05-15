#ifndef DEFAULTAGENT_H
#define DEFAULTAGENT_H

#include "mcpagent.h"

namespace uos_ai {

class DefaultAgent : public MCPAgent
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
    
protected:
    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;
};

}
#endif // DEFAULTAGENT_H
