#ifndef BASENETWORK_H
#define BASENETWORK_H

#include "serverdefs.h"
#include "networkdefs.h"

#include <QHttpMultiPart>

class HttpAccessmanager;
class BaseNetWork : public QObject
{
    Q_OBJECT
public:
    explicit BaseNetWork(const AccountProxy &account);
    virtual ~BaseNetWork();

    struct NetWorkResponse {
        AIServer::ErrorType error;
        QString errorString;
        QByteArray data;

        NetWorkResponse(AIServer::ErrorType errortype, const QString &errorstring, const QByteArray &d)
            : error(errortype)
            , errorString(errorstring)
            , data(d)
        {

        }
    };

signals:
    /**
     * @brief sigAbort
     */
    void requestAborted();

    /**
     * @brief readyReadDeltaContent
     */
    void readyReadDeltaContent(const QByteArray &content);

    /**
     * @brief requestFinished
     */
    void requestFinished();

public:
    /**
     * @brief setTimeOut
     * @param msec
     */
    void setTimeOut(int msec);

    /**
     * @brief getHttpNetworkAccessManager
     * @return
     */
    QSharedPointer<HttpAccessmanager> getHttpNetworkAccessManager(const QString &token) const;

    /**
     * @brief request
     * @param data
     * @param path
     * @param multipart
     * @return
     */
    NetWorkResponse request(const QUrl &url, const QJsonObject &data, QHttpMultiPart *multipart, const QString &token = QString());

protected:
    AccountProxy m_accountProxy;
    int m_timeOut = 15000;
};

#endif // BASENETWORK_H
