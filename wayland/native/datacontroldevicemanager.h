#ifndef DATA_CONTROL_DEVICE_MANAGER_H
#define DATA_CONTROL_DEVICE_MANAGER_H

#include <QObject>
#include <wayland-client.h>
#include "wlr-data-control-unstable-v1-client-protocol.h"

namespace uos_ai {
namespace wayland {

class EventQueue;
class DataControlDeviceV1;
class Seat;

class DataControlDeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DataControlDeviceManager(QObject *parent = nullptr);
    virtual ~DataControlDeviceManager();

    bool isValid() const;
    void setup(zwlr_data_control_manager_v1 *manager);
    void release();
    void destroy();

    void setEventQueue(EventQueue *queue);
    EventQueue *eventQueue();

    DataControlDeviceV1 *getDataDevice(Seat *seat, QObject *parent = nullptr);

    operator zwlr_data_control_manager_v1*();
    operator zwlr_data_control_manager_v1*() const;

Q_SIGNALS:
    void removed();

private:
    class Private;
    QScopedPointer<Private> d;
};

}
}

#endif
