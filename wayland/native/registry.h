#ifndef REGISTRY_H
#define REGISTRY_H

#include <QObject>
#include <wayland-client.h>

namespace uos_ai {
namespace wayland {

class ConnectionThread;
class EventQueue;
class Seat;
class DataControlDeviceManager;

class Registry : public QObject
{
    Q_OBJECT
public:
    explicit Registry(QObject *parent = nullptr);
    ~Registry() override;

    void release();
    void destroy();
    void create(wl_display *display);
    void create(ConnectionThread *connection);
    void setup();

    void setEventQueue(EventQueue *queue);
    EventQueue *eventQueue();

    bool isValid() const;

    Seat *createSeat(quint32 name, quint32 version, QObject *parent = nullptr);
    DataControlDeviceManager *createDataControlDeviceManager(quint32 name, quint32 version, QObject *parent = nullptr);

Q_SIGNALS:
    void seatAnnounced(quint32 name, quint32 version);
    void dataControlDeviceManagerAnnounced(quint32 name, quint32 version);

private:
    class Private;
    QScopedPointer<Private> d;
};

}
}

#endif
