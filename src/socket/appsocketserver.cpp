#include "appsocketserver.h"

#include "serverdefs.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logHttp)

AppSocketServer::AppSocketServer(const QString &id, bool noJson, QObject *parent)
    : QObject(parent)
    , m_serverName(id)
    , m_noJson(noJson)
{

}

AppSocketServer::~AppSocketServer()
{
    if (m_localServer && m_localServer->isListening()) {
        m_localServer->close();
    }
}

bool AppSocketServer::startServer()
{
    if (m_localServer == nullptr) {
        m_localServer = new QLocalServer(this);
        connect(m_localServer, &QLocalServer::newConnection, this, &AppSocketServer::handleNewConnection);
    }

    // 删除旧的套接字文件
    if (!QLocalServer::removeServer(m_serverName)) {
        qCWarning(logHttp) << "Failed to remove server:" << m_localServer->errorString();
    }

    // 开始监听
    if (!m_localServer->listen(m_serverName)) {
        qCCritical(logHttp) << "Failed to start server:" << m_localServer->errorString();
        return false;
    }

    qCDebug(logHttp) << "Server started successfully on" << m_serverName;
    return true;
}

qint64 AppSocketServer::sendWrittenBytes()
{
    return m_bytesWritten;
}

void AppSocketServer::handleNewConnection()
{
    QLocalSocket *clientSocket = m_localServer->nextPendingConnection();
    connect(clientSocket, &QLocalSocket::disconnected, clientSocket, &QLocalSocket::deleteLater);
    connect(clientSocket, &QLocalSocket::readyRead, this, &AppSocketServer::handleReadyRead);
    connect(clientSocket, &QLocalSocket::bytesWritten, this, &AppSocketServer::handleBytesWritten);
    qCDebug(logHttp) << m_serverName << "New client connected";
}

void AppSocketServer::handleReadyRead()
{
    QLocalSocket *clientSocket = qobject_cast<QLocalSocket *>(sender());
    if (!clientSocket)
        return;
}

void AppSocketServer::handleBytesWritten(qint64 bytes)
{
    m_bytesWritten += bytes;
}

void AppSocketServer::sendDataToClient(const QString &data)
{
    QList<QLocalSocket *> connectedSockets = m_localServer->findChildren<QLocalSocket *>();
    if (connectedSockets.isEmpty()) {
        qCWarning(logHttp) << "No clients connected to receive stream data on server:" << m_serverName;
        return;
    }

    QString pureContent;
    if (m_noJson) {
        auto obj = QJsonDocument::fromJson(data.toUtf8()).object();
        if (obj.contains("message")) {
            auto msg = obj.value("message").toObject();
            if (msg.value("chatType").toInt() == ChatTextPlain) {
                pureContent = msg.value("content").toString();
            }
        }
    }

    for (QLocalSocket *socket : connectedSockets) {
        if (socket->state() == QLocalSocket::ConnectedState) {
            socket->write(m_noJson ? pureContent.toUtf8() : data.toUtf8());
            socket->flush();
        }
    }
}
