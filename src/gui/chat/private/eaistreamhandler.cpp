#include "eaistreamhandler.h"
#include "mcpconfigsyncer.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

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
            qCWarning(logAIGUI) << "JS proxy is released before request finish:" << m_cb->getOwner()
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
    qCDebug(logAIGUI) << "Connecting to server:" << m_pipeName;
    m_socket.connectToServer(m_pipeName);
}

void EAiStreamHandler::onDataReady()
{
    QByteArray data = m_socket.readAll();

    if (!m_cb.isNull()) {
        m_cb->notify(QString(data), 0);
    } else {
        qCWarning(logAIGUI) << "Stream callback is null!";
    }
}

void EAiStreamHandler::onError(QLocalSocket::LocalSocketError err)
{
    if (!m_cb.isNull()) {
        if (QLocalSocket::LocalSocketError::PeerClosedError != err) {
            qCWarning(logAIGUI) << "Socket error occurred:" << err;

            switch (err) {
            //Skip unnecessary error code
            case QLocalSocket::LocalSocketError::ServerNotFoundError:
                qCDebug(logAIGUI) << "Server not found error, skipping notification";
                break;
            default:
                m_cb->notify(m_socket.errorString(), -1);
                break;
            }
        }
    } else {
        qCWarning(logAIGUI) << "Stream callback is null!";
    }

    releaseStreamHandler();
}

void EAiStreamHandler::onDisconnected()
{
    qCDebug(logAIGUI) << "Socket disconnected, releasing handler";
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
        qCWarning(logAIGUI) << "EAiStreamHandler already released.";
    }
}
