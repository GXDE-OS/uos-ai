#ifndef EAISTREAMHANDLER_H
#define EAISTREAMHANDLER_H

#include "eaicallbck.h"

#include <QObject>
#include <QSharedPointer>
#include <QLocalSocket>

class EAiStreamHandler : public QObject
{
    Q_OBJECT
public:
    explicit EAiStreamHandler(
        QString pipe,
        QSharedPointer<EAiCallback> callback,
        QObject *parent = nullptr);

    void process();

    enum {
        StreamEnd = 200,
    };
signals:

protected slots:
    void onDataReady();
    void onError(QLocalSocket::LocalSocketError err);
    void onDisconnected();
protected:

    bool m_fReleased {false};
    void releaseStreamHandler();

    QLocalSocket m_socket;
    QString m_pipeName;
    QSharedPointer<EAiCallback> m_cb;
};

#endif // EAISTREAMHANDLER_H
