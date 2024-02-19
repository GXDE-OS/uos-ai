#ifndef ZHIPUNETWORK_H
#define ZHIPUNETWORK_H

#include "basenetwork.h"

class ZhiPuNetWork : public BaseNetWork
{
    Q_OBJECT

public:
    explicit ZhiPuNetWork(const AccountProxy &account);

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
