#include "connection_thread.h"

#include <QAbstractEventDispatcher>
#include <QDebug>
#include <QDir>
#include <QFileSystemWatcher>
#include <QGuiApplication>
#include <QMutex>
#include <QMutexLocker>
#include <QSocketNotifier>
#include <poll.h>

namespace uos_ai {
namespace wayland {

class ConnectionThread::Private
{
public:
    Private(ConnectionThread *q);
    ~Private();
    void doInitConnection();
    void setupSocketNotifier();
    void setupSocketFileWatcher();
    void dispatchEvents();

    wl_display *display = nullptr;
    int fd = -1;
    QString socketName;
    QDir runtimeDir;
    QScopedPointer<QSocketNotifier> socketNotifier;
    QScopedPointer<QFileSystemWatcher> socketWatcher;
    bool serverDied = false;
    bool foreign = false;
    QMetaObject::Connection eventDispatcherConnection;
    int error = 0;
    static QVector<ConnectionThread *> connections;
    static QMutex mutex;

private:
    ConnectionThread *q;
};

QVector<ConnectionThread *> ConnectionThread::Private::connections = QVector<ConnectionThread *>{};
QMutex ConnectionThread::Private::mutex{QMutex::Recursive};

ConnectionThread::Private::Private(ConnectionThread *q)
    : socketName(QString::fromUtf8(qgetenv("WAYLAND_DISPLAY")))
    , runtimeDir(QString::fromUtf8(qgetenv("XDG_RUNTIME_DIR")))
    , q(q)
{
    if (socketName.isEmpty()) {
        socketName = QStringLiteral("wayland-0");
    }
    {
        QMutexLocker lock(&mutex);
        connections << q;
    }
}

ConnectionThread::Private::~Private()
{
    {
        QMutexLocker lock(&mutex);
        connections.removeOne(q);
    }
    if (display && !foreign) {
        wl_display_flush(display);
        wl_display_disconnect(display);
    }
}

void ConnectionThread::Private::doInitConnection()
{
    if (fd != -1) {
        display = wl_display_connect_to_fd(fd);
    } else {
        display = wl_display_connect(socketName.toUtf8().constData());
    }
    if (!display) {
        qWarning() << "Failed connecting to Wayland display";
        Q_EMIT q->failed();
        return;
    }
    if (fd != -1) {
        qDebug() << "Connected to Wayland server over file descriptor:" << fd;
    } else {
        qDebug() << "Connected to Wayland server at:" << socketName;
    }

    setupSocketNotifier();
    setupSocketFileWatcher();
    Q_EMIT q->connected();
}

void ConnectionThread::Private::setupSocketNotifier()
{
    const int fd = wl_display_get_fd(display);
    socketNotifier.reset(new QSocketNotifier(fd, QSocketNotifier::Read));
    QObject::connect(socketNotifier.data(), &QSocketNotifier::activated, q, [this]() {
        dispatchEvents();
    });
}

void ConnectionThread::Private::dispatchEvents()
{
    if (!display) {
        return;
    }
    while (wl_display_prepare_read(display) != 0) {
        wl_display_dispatch_pending(display);
    }
    wl_display_flush(display);
    
    struct pollfd pfd;
    pfd.fd = wl_display_get_fd(display);
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, 0);
    if (ret) {
        wl_display_read_events(display);
    } else {
        wl_display_cancel_read(display);
    }

    if (wl_display_dispatch_pending(display) == -1) {
        error = wl_display_get_error(display);
        if (error != 0) {
            if (display) {
                free(display);
                display = nullptr;
            }
            Q_EMIT q->errorOccurred();
            return;
        }
    }
    Q_EMIT q->eventsRead();
}

void ConnectionThread::Private::setupSocketFileWatcher()
{
    if (!runtimeDir.exists() || fd != -1) {
        return;
    }
    socketWatcher.reset(new QFileSystemWatcher);
    socketWatcher->addPath(runtimeDir.absoluteFilePath(socketName));
    QObject::connect(socketWatcher.data(), &QFileSystemWatcher::fileChanged, q, [this](const QString &file) {
        if (QFile::exists(file) || serverDied) {
            return;
        }
        qWarning() << "Connection to server went away";
        serverDied = true;
        if (display) {
            free(display);
            display = nullptr;
        }
        socketNotifier.reset();

        socketWatcher.reset(new QFileSystemWatcher);
        socketWatcher->addPath(runtimeDir.absolutePath());
        QObject::connect(socketWatcher.data(), &QFileSystemWatcher::directoryChanged, q, [this]() {
            if (!serverDied) {
                return;
            }
            if (runtimeDir.exists(socketName)) {
                qDebug() << "Socket reappeared";
                socketWatcher.reset();
                serverDied = false;
                error = 0;
                q->initConnection();
            }
        });
        Q_EMIT q->connectionDied();
    });
}

ConnectionThread::ConnectionThread(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
    d->eventDispatcherConnection = connect(
        QCoreApplication::eventDispatcher(),
        &QAbstractEventDispatcher::aboutToBlock,
        this,
        [this] {
            if (d->display) {
                wl_display_flush(d->display);
            }
        },
        Qt::DirectConnection);
}

ConnectionThread::ConnectionThread(wl_display *display, QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
    d->display = display;
    d->foreign = true;
}

ConnectionThread::~ConnectionThread()
{
    disconnect(d->eventDispatcherConnection);
}

void ConnectionThread::initConnection()
{
    QMetaObject::invokeMethod(this, &ConnectionThread::doInitConnection, Qt::QueuedConnection);
}

void ConnectionThread::doInitConnection()
{
    d->doInitConnection();
}

void ConnectionThread::setSocketName(const QString &socketName)
{
    if (d->display) {
        return;
    }
    d->socketName = socketName;
}

void ConnectionThread::setSocketFd(int fd)
{
    if (d->display) {
        return;
    }
    d->fd = fd;
}

wl_display *ConnectionThread::display()
{
    return d->display;
}

QString ConnectionThread::socketName() const
{
    return d->socketName;
}

void ConnectionThread::flush()
{
    if (!d->display) {
        return;
    }
    wl_display_flush(d->display);
}

bool ConnectionThread::hasError() const
{
    return d->error != 0;
}

int ConnectionThread::errorCode() const
{
    return d->error;
}

QVector<ConnectionThread *> ConnectionThread::connections()
{
    return Private::connections;
}

}
}
