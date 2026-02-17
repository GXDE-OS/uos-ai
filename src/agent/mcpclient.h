#ifndef MCPCLIENT_H
#define MCPCLIENT_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QProcessEnvironment>

namespace uos_ai {

class McpClient : public QObject
{
    Q_OBJECT
public:
    /**
     * 构造函数
     * @param parent 父对象指针
     */
    explicit McpClient(QObject *parent = nullptr);
    
    /**
     * 析构函数
     * 清理资源，关闭连接
     */
    ~McpClient();

    /**
     * 初始化MCP客户端
     * 加载配置文件，建立与MCP服务器的连接
     * @returns {bool} 初始化是否成功，true表示成功连接，false表示连接失败
     */
    bool init();

    /**
     * 获取智能体可用的MCP服务
     * 查询指定智能体可以访问的所有MCP服务器列表
     * @param {QString} agentName - 智能体名称标识符
     * @returns {QStringList} 服务器名称列表，如果查询失败返回空列表
     */
    QStringList listServers(const QString &agentName);

    /**
     * 获取智能体可用的MCP工具
     * 查询指定智能体可以调用的所有工具定义
     * @param {QString} agentName - 智能体名称标识符
     * @returns {QPair<int, QJsonValue>} 状态码和工具列表
     *          状态码：0表示成功，非0表示错误
     *          QJsonValue：包含工具定义的JSON数组
     */
    QPair<int, QJsonValue> getTools(const QString &agentName);

    /**
     * 调用智能体工具
     * 执行指定的工具调用，传递参数并获取结果
     * @param {QString} agentName - 智能体名称标识符
     * @param {QString} toolName - 要调用的工具名称
     * @param {QJsonObject} params - 工具调用参数的JSON对象
     * @returns {QPair<int, QString>} 状态码和结果内容
     *          状态码：0表示成功，非0表示错误
     *          QString：工具调用的结果内容
     */
    QPair<int, QString> callTool(const QString &agentName, const QString &toolName, const QJsonObject &params);
    
    /**
     * 检查MCP服务器状态
     * 发送ping请求检查服务器是否可用
     * @returns {bool} 服务器是否可用，true表示服务器正常，false表示服务器不可用
     */
    bool ping() const;

    /**
     * 查询指定服务器信息
     * 获取指定服务器列表的详细信息
     * @param {QString} agentName - 智能体名称标识符
     * @param {QStringList} servers - 要查询的服务器名称列表
     * @returns {QPair<int, QJsonValue>} 状态码和服务器信息
     *          状态码：0表示成功，非0表示错误
     *          QJsonValue：包含服务器详细信息的JSON对象
     */
    QPair<int, QJsonValue> queryServers(const QString &agentName, const QStringList &servers);

    /**
     * 同步MCP服务器
     * 与MCP服务器同步状态和配置信息
     * @param {QString} agentName - 智能体名称标识符
     * @returns {QPair<int, QJsonObject>} 状态码和同步结果
     *          状态码：0表示成功，非0表示错误
     *          QJsonObject：同步操作的结果信息
     */
    QPair<int, QJsonObject> syncServers(const QString &agentName);

private:
    /**
     * 读取MCP服务器配置
     * 从配置文件中加载MCP服务器的连接信息
     * @returns {bool} 是否成功读取配置，true表示成功，false表示配置读取失败
     */
    bool loadMcpServerConfig();

    /**
     * 获取状态文件路径
     * 获取用于存储MCP客户端状态的文件路径
     * @returns {QString} 状态文件的完整路径
     */
    QString stateFilePath() const;

    /**
     * 获取基础URL
     * 构建MCP服务器的基础URL地址
     * @returns {QUrl} MCP服务器的基础URL对象
     */
    QUrl baseUrl() const;

    /**
     * 完善环境变量
     * 将默认环境变量与用户自定义环境变量合并
     * @param {QProcessEnvironment} defEnv - 默认环境变量集合
     * @returns {QProcessEnvironment} 完善后的环境变量集合
     */
    QProcessEnvironment perfectEnv(QProcessEnvironment defEnv);

    /**
     * 获取用户环境变量
     * 读取用户自定义的环境变量配置
     * @returns {QVariantHash} 用户环境变量的键值对哈希表
     */
    QVariantHash userEnv() const;

private:
    QString m_serverIp;       // MCP服务器IP地址
    int m_serverPort;         // MCP服务器端口号
    int m_serverPid;          // MCP服务器进程ID，用于进程管理
};

} // namespace uos_ai

#endif // MCPCLIENT_H 
