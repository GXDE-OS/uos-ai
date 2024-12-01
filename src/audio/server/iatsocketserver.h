#ifndef IATSOCKETSERVER_H
#define IATSOCKETSERVER_H

#include "serverdefs.h"
#include "pgsparser.h"
#include "iatserver.h"

#include <QUrl>
#include <QWebSocket>
#include <QTimer>

class IatSocketServer : public IatServer
{
    Q_OBJECT
public:
    explicit IatSocketServer(const AccountProxy &account, QObject *parent = nullptr);

public:
    /**
     * @brief requestAbort
     */
    void cancel() override;

    /**
     * @brief openServer
     */
    void openServer() override;

    /**
     * @brief sendData
     * @param data
     */
    void sendData(const QByteArray &data) override;

private slots:
    /**
     * @brief onTextMessageReceived
     */
    void onTextMessageReceived(const QString &message);

    /**
     * @brief onDisconnected
     */
    void onDisconnected();

    /**
     * @brief processData
     */
    void processData();

private:
    /**
     * @brief clear
     */
    void clear();

    /**
     * @brief normalExitServer
     */
    void normalExitServer();

private:
    AccountProxy m_account;
    QSharedPointer<QWebSocket> m_web;

    bool m_normalExit = false;

    int m_readStatus = 0;
    int m_error = -1;

    QByteArray m_data;
    QTimer m_processTimer;
    QTimer m_abnormalExitTimer;

    PgsParser m_pgsParser;
};

#endif // IATSOCKETSERVER_H
