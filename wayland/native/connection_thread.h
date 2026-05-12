#ifndef CONNECTION_THREAD_H
#define CONNECTION_THREAD_H

#include <QObject>
#include <QVector>
#include <wayland-client.h>

namespace uos_ai {
namespace wayland {

class ConnectionThread : public QObject
{
    Q_OBJECT
public:
    explicit ConnectionThread(QObject *parent = nullptr);
    ~ConnectionThread() override;

    wl_display *display();
    QString socketName() const;
    void setSocketName(const QString &socketName);
    void setSocketFd(int fd);

    bool hasError() const;
    int errorCode() const;

    static QVector<ConnectionThread *> connections();

public Q_SLOTS:
    void initConnection();
    void flush();

Q_SIGNALS:
    void connected();
    void failed();
    void eventsRead();
    void connectionDied();
    void errorOccurred();

protected:
    explicit ConnectionThread(wl_display *display, QObject *parent);

private Q_SLOTS:
    void doInitConnection();

private:
    class Private;
    QScopedPointer<Private> d;
};

}
}

#endif
