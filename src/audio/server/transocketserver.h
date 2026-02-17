#ifndef TranSocketServer_H
#define TranSocketServer_H

#include "serverdefs.h"
#include "pgsparser.h"
#include "transerver.h"

#include <QNetworkAccessManager>

UOSAI_BEGIN_NAMESPACE

class TranSocketServer : public TranServer
{
    Q_OBJECT
public:
    explicit TranSocketServer(const AccountProxy &account, QObject *parent = nullptr);

public:
    void sendText(const QString &text) override;

private:
    /**
     * @brief rootUrlPath
     * @return
     */
    AccountProxy m_account;
    QMap<QString, QString> m_businessArgs;
    QNetworkAccessManager *m_manager;
};

UOSAI_END_NAMESPACE

#endif // TranSocketServer_H
