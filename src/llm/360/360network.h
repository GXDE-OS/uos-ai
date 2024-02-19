#ifndef NETWORK360_H
#define NETWORK360_H

#include "basenetwork.h"

class NetWork360 : public BaseNetWork
{
    Q_OBJECT

public:
    explicit NetWork360(const AccountProxy &account);

public:
    /**
     * @brief rootUrlPath
     * @return
     */
    QString rootUrlPath() const;

    /**
     * @brief request
     * @param data
     * @param path
     * @param multipart
     * @return
     */
    QPair<int, QByteArray> request(const QJsonObject &data, const QString &path, QHttpMultiPart *multipart = nullptr);
};

#endif // NETWORK360_H
