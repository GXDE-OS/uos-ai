#include "mcpclient.h"
#include "osinfo.h"
#include "networkdefs.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QProcess>
#include <QDir>
#include <QLoggingCategory>
#include <QThread>
#include <QLocale>
#include <QStandardPaths>

#include <unistd.h>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {
McpClient::McpClient(QObject *parent)
    : QObject(parent)
    , m_serverIp("127.0.0.1")
    , m_serverPort(0)
    , m_serverPid(0)
{
}

McpClient::~McpClient()
{
}

bool McpClient::init()
{
    static const QString exeName = "uos-aiagent-mcp";
    if (loadMcpServerConfig()) {
        // 测试服务器连接
        if (ping())
            return true;
        else {
            // 检查进程是否存在
            if (m_serverPid > 0) {
                const QString proc = QString("/proc/%0/exe").arg(m_serverPid);
                QFile exeFile(proc);
                if (exeFile.exists()) {
                    QString exePath = exeFile.symLinkTarget();
                    QString exeBaseName = QFileInfo(exePath).fileName();
                    if (exeBaseName == exeName) {
                        qCWarning(logAgent) << "MCP server process exists but ping failed, pid:" << m_serverPid;
                        return true;
                    }
                }
                // 进程不存在，删除状态文件
                QFile::remove(stateFilePath());
                qCInfo(logAgent) << "MCP server process not found, removed state file:" << stateFilePath();
            }
        }
    }

    QProcess process;
    auto defEnv = perfectEnv(UosInfo()->pureEnvironment());

    qCInfo(logAgent) << "Starting agent server with environment" << defEnv.toStringList();
    process.setProcessEnvironment(defEnv);
    process.setProgram(exeName);
    process.setWorkingDirectory(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));

    qint64 pid = -1;
    bool ok = process.startDetached(&pid);
    if (!ok || pid < 1) {
        qCCritical(logAgent) << "Failed to start modelhub server:"
                                 << process.program() << process.arguments() << process.workingDirectory()
                                 << "Error:" << process.errorString();
        return false;
    }

    qCInfo(logAgent) << "Successfully started agent server with pid" << pid;

    // wait server
    {
        const QString proc = QString("/proc/%0").arg(pid);
        int waitCount = 2 * 30; // 等30秒
        while (waitCount-- && QFileInfo::exists(proc)) {
            QThread::msleep(500);
            loadMcpServerConfig();
            if (!m_serverIp.isEmpty() && m_serverPort > 0 && ping()) {
                qCInfo(logAgent) << "agent server ready for at" << m_serverIp << ":" << m_serverPort;
                return true;
            }
        }
    }

    return false;
}

QUrl McpClient::baseUrl() const
{
    return QUrl(QString("http://%1:%2").arg(m_serverIp).arg(m_serverPort));
}

QProcessEnvironment McpClient::perfectEnv(QProcessEnvironment defEnv)
{
    auto user = userEnv();
    const QString uv_index = "UV_DEFAULT_INDEX";
    const QString npm_reg = "NPM_CONFIG_REGISTRY";

    bool cn = false;
    {
        // 获取系统语言
        QLocale locale = QLocale::system();
        QString localeName = locale.name();
        // 判断是否为大陆语言
        cn = localeName == "zh_CN" || localeName == "bo_CN" || localeName == "ug_CN";
    }

    // 如果配置文件中没有设置 UV_DEFAULT_INDEX，则使用默认逻辑
    if (user.value(uv_index).toString().isEmpty()) {
        if (cn)
            defEnv.insert(uv_index, "http://mirrors.aliyun.com/pypi/simple");
    } else {
        defEnv.insert(uv_index, user.value(uv_index).toString());
    }

    // 如果配置文件中没有设置 NPM_CONFIG_REGISTRY，则使用默认逻辑
    if (user.value(npm_reg).toString().isEmpty()) {
        if (cn)
            defEnv.insert(npm_reg, "https://repo.huaweicloud.com/repository/npm");
    } else {
        defEnv.insert(npm_reg, user.value(npm_reg).toString());
    }

#ifdef QT_DEBUG
    defEnv.insert("LOG_LEVEL", "DEBUG");
    defEnv.insert("AGENT_SERVER_PORT", "38275");
#endif

    if (!user.value("LOG_LEVEL").toString().isEmpty())
        defEnv.insert("LOG_LEVEL", user.value("LOG_LEVEL").toString());

    return defEnv;
}

QVariantHash McpClient::userEnv() const
{
    // 读取配置文件
    QString configPath = QDir::homePath() + "/.config/deepin/uos-ai-assistant/mcp-env.conf";
    QFile configFile(configPath);

    if (configFile.exists() && configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
        configFile.close();

        if (!doc.isNull() && doc.isObject()) {
            QJsonObject envVars = doc.object();
            return envVars.toVariantHash();
        }
    }
    return {};
}

bool McpClient::loadMcpServerConfig()
{   
    // 读取MCP服务配置
    QFile file(stateFilePath());
    
    if (!file.exists()) {
        qCWarning(logAgent) << "MCP server configuration file not found:" << file.fileName();
        return false;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qCCritical(logAgent) << "Failed to open MCP server configuration file:" << file.fileName();
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (doc.isNull() || !doc.isObject()) {
        qCWarning(logAgent) << "Invalid MCP server configuration format in file:" << file.fileName();
        return false;
    }
    
    QJsonObject config = doc.object();
    
    if (config.contains("ip")) {
        m_serverIp = config["ip"].toString();
    }
    
    if (config.contains("port")) {
        m_serverPort = config["port"].toInt();
    }
    
    if (config.contains("pid")) {
        m_serverPid = config["pid"].toInt();
    }
    
    if (m_serverPort <= 0) {
        qCCritical(logAgent) << "Invalid MCP server port in configuration:" << m_serverPort;
        return false;
    }
    
    return true;
}

QString McpClient::stateFilePath() const
{
    return QString("/tmp/uos-ai-agent-%1/mcp/server.json").arg(getuid());
}

QPair<int, QJsonValue> McpClient::queryServers(const QString &agentName, const QStringList &servers)
{
    QUrl url = baseUrl().resolved(QUrl("query_server"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject requestData;
    requestData["agent_name"] = agentName;
    if (servers.isEmpty())
        requestData["server_names"] = "";
    else
        requestData["server_names"] = QJsonArray::fromStringList(servers);

    QJsonDocument requestDoc(requestData);
    
    QNetworkAccessManager networkManager;
    QNetworkReply *reply = networkManager.post(request, requestDoc.toJson());
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logAgent) << "Error fetching MCP servers for agent" << agentName 
                           << "Error:" << reply->errorString();
        reply->deleteLater();
        return qMakePair(AIServer::ErrorType::AgentServerUnavailable, QJsonObject());
    }
    
    QByteArray responseData = reply->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    reply->deleteLater();
    
    if (responseDoc.isNull() || !responseDoc.isObject()) {
        qCWarning(logAgent) << "Invalid MCP server response format from" << agentName << servers;
        return qMakePair(AIServer::ErrorType::AgentServerInvaildContent, QJsonObject());
    }
    
    QJsonObject responseObj = responseDoc.object();
    
    if (!responseObj.contains("servers") || !responseObj["servers"].isArray()) {
        qWarning() << "Invalid MCP server response: servers array not found" << agentName << servers;
        return qMakePair(AIServer::ErrorType::AgentServerInvaildContent, QJsonObject());
    }
    
    QJsonArray serversArray = responseObj["servers"].toArray();
    return qMakePair(AIServer::ErrorType::NoError, serversArray);
}

QPair<int, QJsonValue> McpClient::getTools(const QString &agentName)
{
    QPair<int, QJsonValue> srv = queryServers(agentName, {});
    if (srv.first != 0)
        return srv;

    QJsonArray toolsArray;
    for (const QJsonValue &serverValue : srv.second.toArray()) {
        if (!serverValue.isObject()) {
            continue;
        }

        QJsonObject serverObj = serverValue.toObject();

        if (serverObj.contains("tools") && serverObj["tools"].isArray()) {
            QJsonArray serverTools = serverObj["tools"].toArray();
            for (const QJsonValue &toolValue : serverTools) {
                toolsArray.append(toolValue);
            }
        }
    }

    return qMakePair(AIServer::ErrorType::NoError, toolsArray);
}

QPair<int, QString> McpClient::callTool(const QString &agentName, const QString &toolName, const QJsonObject &params)
{
    QUrl url = baseUrl().resolved(QUrl("call_tool"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject requestData;
    requestData["agent_name"] = agentName;
    requestData["tool_name"] = toolName;
    requestData["params"] = params;
    QJsonDocument requestDoc(requestData);
    
    QNetworkAccessManager networkManager;
    QNetworkReply *reply = networkManager.post(request, requestDoc.toJson());
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        QString erInfo = QString::fromUtf8(reply->readAll());
        qCWarning(logAgent) << "Error calling MCP tool:" << toolName << "for agent:" << agentName
                           << "Error:" << reply->errorString() << "Response:" << erInfo;
        reply->deleteLater();
        return qMakePair(AIServer::ErrorType::MCPSeverUnavailable, erInfo);
    }
    
    QByteArray responseData = reply->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    reply->deleteLater();
    
    if (responseDoc.isNull() || !responseDoc.isObject()) {
        qWarning() << "Invalid MCP tool response format" << agentName << toolName;
        return qMakePair(AIServer::ErrorType::AgentServerInvaildContent, QString());
    }
    
    QJsonObject responseObj = responseDoc.object();
    
    if (!responseObj.contains("result")) {
        qWarning() << "Invalid MCP tool response: result not found" << agentName << toolName;
        return qMakePair(AIServer::ErrorType::AgentServerInvaildContent, QString());
    }
    
    bool toolEr = false;
    if (responseObj.contains("isError"))
        toolEr = responseObj["isError"].toBool(false);

    return qMakePair(toolEr ? AIServer::ErrorType::MCPToolError : AIServer::ErrorType::NoError,
                     responseObj["result"].toString());
}

bool McpClient::ping() const
{
    QUrl url = baseUrl().resolved(QUrl("ping"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkAccessManager networkManager;
    QNetworkReply *reply = networkManager.get(request);
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logAgent) << "Ping failed for MCP server at" << url.toString()
                           << "Error:" << reply->errorString();
        reply->deleteLater();
        return false;
    }
    
    reply->deleteLater();
    return true;
}

QStringList McpClient::listServers(const QString &agentName)
{
    QStringList ret;

    QUrl url = baseUrl().resolved(QUrl("list_servers"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject requestData;
    requestData["agent_name"] = agentName;

    QJsonDocument requestDoc(requestData);

    QNetworkAccessManager networkManager;
    QNetworkReply *reply = networkManager.post(request, requestDoc.toJson());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logAgent) << "Error listing MCP servers for agent" << agentName
                           << "Error:" << reply->errorString();
        reply->deleteLater();
        return ret;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    reply->deleteLater();

    if (responseDoc.isNull() || !responseDoc.isObject()) {
        qCWarning(logAgent) << "Invalid MCP servers response format from listing servers" << agentName;
        return ret;
    }

    QJsonObject responseObj = responseDoc.object();

    if (!responseObj.contains("servers") || !responseObj["servers"].isArray()) {
        qWarning() << "Invalid MCP server response: servers array not found" << agentName;
        return ret;
    }

    QJsonArray serversArray = responseObj["servers"].toArray();

    for (const QJsonValue &v : serversArray) {
        auto name = v.toString().trimmed();
        if (!name.isEmpty())
            ret.append(name);
    }

    return ret;
}

QPair<int, QJsonObject> McpClient::syncServers(const QString &agentName)
{
    QUrl url = baseUrl().resolved(QUrl("sync_servers"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject requestData;
    requestData["agent_name"] = agentName;
    QJsonDocument requestDoc(requestData);
    
    QNetworkAccessManager networkManager;
    QNetworkReply *reply = networkManager.post(request, requestDoc.toJson());
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logAgent) << "Error syncing servers for agent" << agentName 
                           << "Error:" << reply->errorString();
        reply->deleteLater();
        return qMakePair(AIServer::ErrorType::AgentServerUnavailable, QJsonObject());
    }
    
    QByteArray responseData = reply->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    reply->deleteLater();
    
    if (responseDoc.isNull() || !responseDoc.isObject()) {
        qCWarning(logAgent) << "Invalid sync servers response format from" << agentName;
        return qMakePair(AIServer::ErrorType::AgentServerInvaildContent, QJsonObject());
    }
    
    QJsonObject responseObj = responseDoc.object();    
    return qMakePair(AIServer::ErrorType::NoError, responseObj);
}

} // namespace uos_ai 
