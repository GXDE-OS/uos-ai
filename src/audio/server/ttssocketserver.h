#ifndef TTSSOCKETSERVER_H
#define TTSSOCKETSERVER_H

#include "serverdefs.h"
#include "ttsserver.h"

#include <QWebSocket>

class TtsSocketServer : public TtsServer
{
    Q_OBJECT
public:
    explicit TtsSocketServer(const QString &id, const AccountProxy &account, QObject *parent = nullptr);

public:
    /**
     * @brief cancel
     */
    void cancel() override;

    /**
     * @brief openServer
     */
    void openServer() override;

    /**
     * @brief sendText
     * @param text
     */
    void sendText(const QString &text, bool isStart, bool isEnd) override;

private slots:
    /**
     * @brief onTextMessageReceived
     * @param message
     */
    void onTextMessageReceived(const QString &message);

    /**
     * @brief onDisconnected
     */
    void onDisconnected();

    /**
     * @brief onConnected
     */
    void onConnected();

    /**
     * @brief continueSendText
     */
    void continueSendText();

private:
    /**
     * @brief clear
     */
    void clear();

    /**
     * @brief sendServer
     */
    void sendServer();

private:
    AccountProxy m_account;
    QSharedPointer<QWebSocket> m_web;

    int m_error = -1;

    bool m_normalExit = false;
    bool m_isEnd = false;
    QString m_text;

    QStringList m_chunkTexts;
};

#endif // TTSSOCKETSERVER_H
