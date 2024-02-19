#ifndef WXQFNETWORK_H
#define WXQFNETWORK_H

#include "basenetwork.h"

class WXQFNetWork : public BaseNetWork
{
    Q_OBJECT

public:
    explicit WXQFNetWork(const AccountProxy &account);

public:
    /**
     * @brief request
     * @param data
     * @param path
     * @param multipart
     * @return
     */
    QPair<int, QByteArray> request(const QJsonObject &data, const QString &urlPath, QHttpMultiPart *multipart = nullptr);

private:
    /**
     * @brief generateAccessToken
     * @param clientId
     * @param clientSecret
     * @return
     */
    QPair<int, QString> generateAccessToken(const QString &clientId, const QString &clientSecret) const;
};

#endif // NETWORK360_H
