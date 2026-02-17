#ifndef MCPSERVER_H
#define MCPSERVER_H

#include <QObject>
#include <QStringList>
#include <QVariantHash>

namespace uos_ai {

/**
 * MCP服务管理器，用于管理MCP服务的扫描、添加和删除
 */
class MCPServer : public QObject
{
    Q_OBJECT
public:
    explicit MCPServer(const QString &agentName, QObject *parent = nullptr);

    /**
     * MCP 环境是否有效
     */
    virtual bool isRuntimeReady() const;

    /**
     * 扫描所有MCP服务
     */
    virtual void scanServers();
    
    /**
     * 获取所有服务名
     * @returns {QStringList} 服务列表
     */
    virtual QStringList serverNames() const;

    /**
     * 更新自定义服务配置
     * @param {QString} cfgContent - 配置内容
     * @returns {QPair<bool, QString>} 是否成功和错误信息
     */
    virtual QPair<bool, QString> updateCustomServers(const QString &cfgContent);

    /**
     * 导入自定义服务配置
     * @param {QString} cfgContent - 配置内容
     * @returns {QPair<bool, QString>} 是否成功和错误信息
     */
    virtual QPair<bool, QString> importCustomServers(const QString &cfgContent);

    /**
     * 添加自定义服务
     * @param {QString} name - 服务名称
     * @param {QJsonObject} serverInfo - 服务信息
     * @returns {QPair<bool, QString>} 是否成功和错误信息
     */
    virtual QPair<bool, QString> addCustomServer(const QString &name, const QJsonObject &serverInfo);
    
    /**
     * 删除自定义服务
     * @param {QString} name - 服务名称
     * @returns {QPair<bool, QString>} 是否成功和错误信息
     */
    virtual QPair<bool, QString> removeCustomServer(const QString &name);

    virtual bool isRemovable(const QString &name) const;

    virtual QString description(const QString &name) const;

    virtual bool isBuiltin(const QString &name) const;

    virtual QJsonObject serverConfig(const QString &name) const;
protected:
    /**
     * 扫描指定目录中的服务
     * @param {QString} path - 目录路径
     */
    virtual void scanDirectory(const QString &path);
    
    /**
     * 检查服务名是否已存在
     * @param {QString} name - 服务名称
     * @returns {bool} 是否存在
     */
    bool isServerNameExist(const QString &name) const;
    
    /**
     * 检查服务信息是否有效
     * @param {QJsonObject} serverInfo - 服务信息
     * @returns {QPair<bool, QString>} 是否有效和错误信息
     */
    virtual QPair<bool, QString> validateServerInfo(const QJsonObject &serverInfo) const;

protected:
    QString m_agentName;
    QMap<QString, QVariantHash> m_servers; // <serverName, info>
    QString m_customConfigPath;
};

} // namespace uos_ai

#endif // MCPSERVER_H
