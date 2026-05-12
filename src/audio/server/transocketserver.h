#ifndef TranSocketServer_H
#define TranSocketServer_H

#include "pgsparser.h"
#include "transerver.h"
#include "model/modelinfo.h"

#include <QNetworkAccessManager>

namespace uos_ai {

class TranSocketServer : public TranServer
{
    Q_OBJECT
public:
    explicit TranSocketServer(const ProviderAccount &account, QObject *parent = nullptr);

public:
    void sendText(const QString &text) override;

private:
    /**
     * @brief rootUrlPath
     * @return
     */
    ProviderAccount m_account;
    QMap<QString, QString> m_businessArgs;
    QNetworkAccessManager *m_manager;
};

}

#endif // TranSocketServer_H
