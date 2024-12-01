#ifndef AINETWORK_H
#define AINETWORK_H

#include "basenetwork.h"

class AINetWork : public BaseNetWork
{
    Q_OBJECT

public:
    explicit AINetWork(const AccountProxy &account);

public:
    /**
     * @brief rootUrlPath
     * @return
     */
    virtual QString rootUrlPath() const;

    /**
     * @brief request
     * @param data
     * @param path
     * @param multipart
     * @return
     */
    QPair<int, QByteArray> request(const QJsonObject &data, const QString &path, QHttpMultiPart *multipart = nullptr);
};

#endif // AINETWORK_H
