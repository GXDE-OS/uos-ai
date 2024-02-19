#ifndef IATLOCALSERVER_H
#define IATLOCALSERVER_H

#include "iatserver.h"
#include "dbuslocalspeechrecognitionrequest.h"

#include <QTimer>

class IatLocalServer : public IatServer
{
    Q_OBJECT
public:
    explicit IatLocalServer(QObject *parent = nullptr);

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
    void onTextMessageReceived(const QString &message, bool isEnd);

    /**
     * @brief processData
     */
    void processData();

private:
    /**
     * @brief normalExitServer
     */
    void normalExitServer();

private:
    bool m_isOpen = false;

    QByteArray m_data;
    QTimer m_processTimer;

    DbusLocalSpeechRecognitionRequest m_dbus;
};

#endif // IATLOCALSERVER_H
