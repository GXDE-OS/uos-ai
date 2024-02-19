#include "eaistreamhandler.h"

#include <QDebug>

EAiStreamHandler::EAiStreamHandler(
    QString pipe,
    QSharedPointer<EAiCallback> callback,
    QObject *parent)
    : QObject{parent}
    , m_pipeName(pipe)
    , m_cb(callback)
{
    connect(&m_socket, &QLocalSocket::readyRead,
            this, &EAiStreamHandler::onDataReady);
#if QT_VERSION <= QT_VERSION_CHECK(5, 15, 0)
    connect(&m_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error),
            this, &EAiStreamHandler::onError);
#else
    connect(&m_socket, &QLocalSocket::errorOccurred,
            this, &EAiStreamHandler::onError);
#endif
    connect(&m_socket, &QLocalSocket::disconnected,
            this, &EAiStreamHandler::onDisconnected);

    /*Need monitor the EAiProxy's Object life cycle.
     *When the native window is closed, the JsAiProxy
     *Object is released,We need stop the stream and stop
     *notify message to the JsAiProxy Object.
     */
    if (!m_cb.isNull()) {
        connect(m_cb->getOwner(), &QObject::destroyed, this, [this]() {
            qWarning() << "JS proxy is released before request finish:" << m_cb->getOwner()
                       << "\n1.Prepare to Close stream."
                       << "\n2.Set callback owner to null.";
            m_socket.close();
            m_cb->setOwner(nullptr);
            m_cb.reset(nullptr);
        });
    }
}

void EAiStreamHandler::process()
{
    m_socket.connectToServer(m_pipeName);
}

void EAiStreamHandler::onDataReady()
{
    QByteArray data = m_socket.readAll();

    if (!m_cb.isNull()) {
        m_cb->notify(QString(data), 0);
    } else {
        qWarning() << "Stream callback is null!";
    }
}

void EAiStreamHandler::onError(QLocalSocket::LocalSocketError err)
{
    if (!m_cb.isNull()) {
        if (QLocalSocket::LocalSocketError::PeerClosedError != err) {
            qInfo() << " error=" << err;

            switch (err) {
            //Skip unnecessary error code
            case QLocalSocket::LocalSocketError::ServerNotFoundError:
                break;
            default:
                m_cb->notify(m_socket.errorString(), -1);
                break;
            }
        }
    } else {
        qWarning() << "Stream callback is null!";
    }

    releaseStreamHandler();
}

void EAiStreamHandler::onDisconnected()
{
    //Release the EAiStreamHandler after
    //connection is closed
    releaseStreamHandler();
}

void EAiStreamHandler::releaseStreamHandler()
{
    if (!m_fReleased) {
        m_fReleased = true;
        deleteLater();
    } else {
        qWarning() << "EAiStreamHandler already released.";
    }
}
