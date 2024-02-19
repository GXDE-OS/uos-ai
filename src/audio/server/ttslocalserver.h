#ifndef TTSLOCALSERVER_H
#define TTSLOCALSERVER_H

#include "serverdefs.h"
#include "ttsserver.h"
#include "dbuslocalspeechrecognitionrequest.h"

class TtsLocalServer : public TtsServer
{
    Q_OBJECT

public:
    explicit TtsLocalServer(const QString &id, QObject *parent = nullptr);

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
    void onTextMessageReceived(const QString &id, const QByteArray &text, bool isEnd);

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

    /**
     * @brief normalExitServer
     */
    void normalExitServer();

private:
    bool m_isOpen = false;
    bool m_isEnd = false;

    QString m_text;
    QStringList m_chunkTexts;

    DbusLocalSpeechRecognitionRequest m_dbus;
};

#endif // TTSLOCALSERVER_H
