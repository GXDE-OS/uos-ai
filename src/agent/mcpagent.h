#ifndef MCPAGENT_H
#define MCPAGENT_H

#include "llmagent.h"

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QQueue>
#include <QFuture>
#include <QtConcurrent>

namespace uos_ai {

class McpClient;
class MCPServer;

class MCPAgent : public LlmAgent
{
    Q_OBJECT
public:
    /**
     * 构造函数
     * @param parent 父对象指针
     */
    explicit MCPAgent(QObject *parent = nullptr);
    
    /**
     * 析构函数
     * 清理资源，关闭连接
     */
    ~MCPAgent() override;

    /**
     * 获取智能体的MCPServer类
     * @returns {MCPServer} MCPServer对象的共享指针
     */
    virtual QSharedPointer<MCPServer> mcpServer() const;

    /**
     * 初始化客户端连接
     * 建立与MCP服务器的连接，加载配置信息
     * @returns {bool} 初始化是否成功，true表示成功，false表示失败
     */
    bool initialize() override;
    
    /**
     * 获取可用的服务器列表
     * 查询当前智能体可以访问的所有MCP服务器
     * @returns {QStringList} 服务器名称列表，如果没有可用服务器则返回空列表
     */
    virtual QStringList listServers() const;
    
    /**
     * 获取可用的工具列表
     * 从所有连接的MCP服务器获取可用的工具定义
     * @returns {QPair<int, QJsonValue>} 状态码和工具定义数组
     *          状态码：0表示成功，非0表示错误
     *          QJsonValue：包含工具定义的JSON数组
     */
    virtual QPair<int, QJsonValue> listTools() const;

    /**
     * 获取工具信息
     * 从指定的服务器列表中获取详细的工具信息
     * @param {QStringList} servers - 要查询的服务器列表
     * @returns {QPair<int, QString>} 状态码和工具信息的JSON字符串
     *          状态码：0表示成功，非0表示错误
     *          QString：包含工具信息的JSON字符串
     */
    virtual QPair<int, QString> fetchTools(const QStringList &servers);

    /**
     * 刷新的服务器列表
     * 刷新MCP服务配置，加载新安装的服务，移除被卸载的服务。
     * @returns {bool} 是否成功
     */
    virtual bool syncServers() const;

    /**
     * 处理用户请求
     * 处理来自用户的请求，包括文本对话和工具调用
     * @param {QJsonObject} question - 当前请求内容，包含用户的问题和相关参数
     * @param {QJsonArray} history - 聊天消息记录，包含之前的对话历史
     * @param {QJsonObject} params - 扩展参数，包含额外的配置信息
     * @returns {QJsonObject} 智能体工作流输出的消息记录
     */
    QJsonObject processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params = {}) override;
protected:
    /**
     * 调用工具
     * 重写LlmAgent的callTool方法，使用MCP客户端调用工具
     * @param {QString} toolName - 要调用的工具名称
     * @param {QJsonObject} params - 工具调用所需的参数
     * @returns {QPair<int, QString>} 工具调用的结果，包含状态码和返回内容
     *          状态码：0表示成功，非0表示错误
     *          QString：工具调用的返回内容或错误信息
     */
    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;

    /**
     * 同步调用MCP客户端的方法，避免事件循环嵌套
     * 在单独的线程中执行函数，避免阻塞主事件循环
     * @param {std::function<T()>} func - 要在工作线程中执行的函数对象
     * @returns {T} 函数执行结果
     * @note 此方法会阻塞当前线程直到函数执行完成
     */
    template<typename T>
    T syncCall(std::function<T()> func) const {
        T result;
        QThread *thread = QThread::create([&func, &result]() {
            result = func();
        });

        thread->start();
        thread->wait();
        delete thread;
        return result;
    }

protected:
    McpClient *m_mcpClient = nullptr;       // MCP客户端实例，用于与MCP服务器通信
    mutable QSet<QString> m_toolList;       // 可用工具列表的缓存，避免重复查询
};

} // namespace uos_ai

#endif // MCPAGENT_H 
