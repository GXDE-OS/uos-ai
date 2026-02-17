#ifndef IATIFLYMODEL_H
#define IATIFLYMODEL_H

#include "serverdefs.h"
#include "pgsparser.h"
#include "../common/iatmodel.h"

#include <QUrl>
#include <QWebSocket>
#include <QTimer>

namespace uos_ai {
class IatIflyModel : public IatModel {
    Q_OBJECT
public:
    explicit IatIflyModel(QObject *parent = nullptr);

public:
    /**
     * @brief openServer
     */
    void openServer();

    void inputEnd();

    /**
     * @brief sendDataStart
     */
    void sendDataStart() override;

    /**
     * @brief sendData
     */
    void sendData(const QByteArray &data) override;

    /**
     * @brief sendDataEnd
     */
    void sendDataEnd() override;

    /**
     * @brief processData
     */
    void processData() override;

    /**
     * @brief requestAbort
     */
    void cancel() override;

private slots:
    /**
     * @brief onTextMessageReceived
     */
    void onTextMessageReceived(const QString &message);

    /**
     * @brief onDisconnected
     */
    void onDisconnected();

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
}

#endif // IATIFLYMODEL_H
