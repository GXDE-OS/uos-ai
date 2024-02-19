#ifndef APPSOCKETSERVER_H
#define APPSOCKETSERVER_H

#include <QObject>

class QLocalServer;
class AppSocketServer : public QObject
{
    Q_OBJECT
public:
    explicit AppSocketServer(const QString &id, QObject *parent = nullptr);
    ~AppSocketServer();

public:
    /**
     * @brief startServer
     * @return
     */
    bool startServer();

    /**
     * @brief sendWrittenBytes
     * @return
     */
    qint64 sendWrittenBytes();

public slots:
    /**
     * @brief sendDataToClient
     * @param data
     */
    void sendDataToClient(const QString &data);

private slots:
    /**
     * @brief handleNewConnection
     */
    void handleNewConnection();

    /**
     * @brief handleReadyRead
     */
    void handleReadyRead();

    /**
     * @brief handleBytesWritten
     */
    void handleBytesWritten(qint64 bytes);

private:
    qint64 m_bytesWritten = 0;
    QLocalServer *m_localServer = nullptr;
    QString m_serverName;
};

#endif // APPSOCKETSERVER_H
