#ifndef XFNETWORK_H
#define XFNETWORK_H

#include "serverdefs.h"
#include "xfconversation.h"

#include <QWebSocket>

class XFNetWork : public QObject
{
    Q_OBJECT
public:
    explicit XFNetWork(const AccountProxy &account, QObject *parent = nullptr);

    QString version(int model) const;

    QJsonObject header() const;

    QJsonObject parameter(int model, qreal temperature) const;

    QJsonObject payloadMessage(const XFConversation &conversation) const;

    QJsonObject payloadFunctions(const QJsonArray &functions) const;

signals:
    /**
     * @brief requestCanceled
     */
    void requestAborted();

    /**
     * @brief readyReadDeltaContent
     * @param content
     */
    void readyReadDeltaContent(const QByteArray &content);

public:
    /**
     * @brief request
     * @param data
     * @param path
     * @param multipart
     * @return
     */
    QPair<int, QByteArray> wssRequest(const QByteArray &sendData, const QString &path);

    /**
     * @brief httpRequest
     * @param sendData
     * @param path
     * @return
     */
    QPair<int, QByteArray> httpRequest(const QJsonObject &data, const QString &path);

protected:
    AccountProxy m_accountProxy;
};

#endif // XFNETWORK_H
