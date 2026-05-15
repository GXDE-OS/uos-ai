#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <QObject>
#include <wayland-client.h>

namespace uos_ai {
namespace wayland {

class ConnectionThread;

class EventQueue : public QObject
{
    Q_OBJECT
public:
    explicit EventQueue(QObject *parent = nullptr);
    ~EventQueue() override;

    void setup(wl_display *display);
    void setup(ConnectionThread *connection);

    bool isValid();
    void release();
    void destroy();

    void addProxy(wl_proxy *proxy);

    template<typename wl_interface>
    void addProxy(wl_interface *proxy)
    {
        addProxy(reinterpret_cast<wl_proxy *>(proxy));
    }

    template<typename wl_interface, typename T>
    void addProxy(T *proxy)
    {
        addProxy(reinterpret_cast<wl_proxy *>((wl_interface *)*(proxy)));
    }

    operator wl_event_queue *();
    operator wl_event_queue *() const;

public Q_SLOTS:
    void dispatch();

private:
    class Private;
    QScopedPointer<Private> d;
};

}
}

#endif
