#include "registry.h"
#include "connection_thread.h"
#include "event_queue.h"
#include "seat.h"
#include "datacontroldevicemanager.h"
#include "wayland_pointer_p.h"

#include <wayland-client.h>

namespace uos_ai {
namespace wayland {

class Registry::Private
{
public:
    Private(Registry *q);
    void setup();
    void handleAnnounce(uint32_t name, const char *interface, uint32_t version);
    void handleRemove(uint32_t name);
    static void globalAnnounce(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    static void globalRemove(void *data, struct wl_registry *registry, uint32_t name);

    WaylandPointer<wl_registry, wl_registry_destroy> registry;
    EventQueue *queue = nullptr;

private:
    Registry *q;
    static const wl_registry_listener s_listener;
};

const wl_registry_listener Registry::Private::s_listener = {
    globalAnnounce,
    globalRemove
};

Registry::Private::Private(Registry *q)
    : q(q)
{
}

void Registry::Private::globalAnnounce(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    auto r = reinterpret_cast<Registry::Private *>(data);
    Q_ASSERT(r->registry == registry);
    r->handleAnnounce(name, interface, version);
}

void Registry::Private::globalRemove(void *data, struct wl_registry *registry, uint32_t name)
{
    auto r = reinterpret_cast<Registry::Private *>(data);
    Q_ASSERT(r->registry == registry);
    r->handleRemove(name);
}

void Registry::Private::handleAnnounce(uint32_t name, const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_seat") == 0) {
        Q_EMIT q->seatAnnounced(name, version);
    } else if (strcmp(interface, "zwlr_data_control_manager_v1") == 0) {
        Q_EMIT q->dataControlDeviceManagerAnnounced(name, version);
    }
}

void Registry::Private::handleRemove(uint32_t name)
{
    Q_UNUSED(name);
}

void Registry::Private::setup()
{
    wl_registry_add_listener(registry, &s_listener, this);
}

Registry::Registry(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
}

Registry::~Registry()
{
    release();
}

void Registry::release()
{
    d->registry.release();
}

void Registry::destroy()
{
    d->registry.destroy();
}

void Registry::create(wl_display *display)
{
    Q_ASSERT(display);
    Q_ASSERT(!d->registry.isValid());
    d->registry.setup(wl_display_get_registry(display));
}

void Registry::create(ConnectionThread *connection)
{
    create(connection->display());
}

void Registry::setup()
{
    d->setup();
}

void Registry::setEventQueue(EventQueue *queue)
{
    d->queue = queue;
}

EventQueue *Registry::eventQueue()
{
    return d->queue;
}

bool Registry::isValid() const
{
    return d->registry.isValid();
}

Seat *Registry::createSeat(quint32 name, quint32 version, QObject *parent)
{
    Q_ASSERT(isValid());
    Seat *seat = new Seat(parent);
    auto w = wl_registry_bind(d->registry, name, &wl_seat_interface, std::min(version, 5u));
    if (d->queue) {
        d->queue->addProxy(w);
    }
    seat->setup((wl_seat *)w);
    return seat;
}

DataControlDeviceManager *Registry::createDataControlDeviceManager(quint32 name, quint32 version, QObject *parent)
{
    Q_ASSERT(isValid());
    DataControlDeviceManager *manager = new DataControlDeviceManager(parent);
    auto w = wl_registry_bind(d->registry, name, &zwlr_data_control_manager_v1_interface, std::min(version, 2u));
    if (d->queue) {
        d->queue->addProxy(w);
    }
    manager->setup((zwlr_data_control_manager_v1 *)w);
    return manager;
}

}
}
