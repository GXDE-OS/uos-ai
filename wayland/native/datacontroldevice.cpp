#include "datacontroldevice.h"
#include "datacontroloffer.h"
#include "wayland_pointer_p.h"

#include <wlr-data-control-unstable-v1-client-protocol.h>

namespace uos_ai {
namespace wayland {

class DataControlDeviceV1::Private
{
public:
    explicit Private(DataControlDeviceV1 *q);
    void setup(zwlr_data_control_device_v1 *d);

    WaylandPointer<zwlr_data_control_device_v1, zwlr_data_control_device_v1_destroy> device;
    QScopedPointer<DataControlOfferV1> selectionOffer;
    QScopedPointer<DataControlOfferV1> primarySelectionOffer;

private:
    void dataContorlOffer(zwlr_data_control_offer_v1 *id);
    void selection(zwlr_data_control_offer_v1 *id);
    void finished();
    void primarySelection(zwlr_data_control_offer_v1 *id);

    static void dataContorlOfferCallback(void *data, zwlr_data_control_device_v1 *dataDevice, zwlr_data_control_offer_v1 *id);
    static void selectionCallback(void *data, zwlr_data_control_device_v1 *dataDevice, zwlr_data_control_offer_v1 *id);
    static void finishedCallback(void *data, zwlr_data_control_device_v1 *dataDevice);
    static void primarySelectionCallback(void *data, zwlr_data_control_device_v1 *dataDevice, zwlr_data_control_offer_v1 *id);

    static const struct zwlr_data_control_device_v1_listener s_listener;

    DataControlDeviceV1 *q;
    DataControlOfferV1 *lastOffer = nullptr;
};

const zwlr_data_control_device_v1_listener DataControlDeviceV1::Private::s_listener = {
    dataContorlOfferCallback,
    selectionCallback,
    finishedCallback,
    primarySelectionCallback
};

void DataControlDeviceV1::Private::dataContorlOfferCallback(void *data, zwlr_data_control_device_v1 *dataDevice, zwlr_data_control_offer_v1 *id)
{
    auto d = reinterpret_cast<Private*>(data);
    Q_ASSERT(d->device == dataDevice);
    d->dataContorlOffer(id);
}

void DataControlDeviceV1::Private::dataContorlOffer(zwlr_data_control_offer_v1 *id)
{
    Q_ASSERT(!lastOffer);
    lastOffer = new DataControlOfferV1(q, id);
    Q_ASSERT(lastOffer->isValid());
    Q_EMIT q->dataControlOffered(lastOffer);
}

void DataControlDeviceV1::Private::selectionCallback(void *data, zwlr_data_control_device_v1 *dataDevice, zwlr_data_control_offer_v1 *id)
{
    auto d = reinterpret_cast<Private*>(data);
    Q_ASSERT(d->device == dataDevice);
    d->selection(id);
}

void DataControlDeviceV1::Private::selection(zwlr_data_control_offer_v1 *id)
{
    if (!id) {
        selectionOffer.reset();
        Q_EMIT q->selectionCleared();
        return;
    }
    Q_ASSERT(*lastOffer == id);
    selectionOffer.reset(lastOffer);
    lastOffer = nullptr;
    Q_EMIT q->dataOffered(selectionOffer.data());
}

void DataControlDeviceV1::Private::finishedCallback(void *data, zwlr_data_control_device_v1 *dataDevice)
{
    auto d = reinterpret_cast<Private*>(data);
    Q_ASSERT(d->device == dataDevice);
    d->finished();
}

void DataControlDeviceV1::Private::finished()
{

}

void DataControlDeviceV1::Private::primarySelectionCallback(void *data, zwlr_data_control_device_v1 *dataDevice, zwlr_data_control_offer_v1 *id)
{
    auto d = reinterpret_cast<Private*>(data);
    Q_ASSERT(d->device == dataDevice);
    d->primarySelection(id);
}

void DataControlDeviceV1::Private::primarySelection(zwlr_data_control_offer_v1 *id)
{
    if (!id) {
        primarySelectionOffer.reset();
        Q_EMIT q->selectionCleared();
        return;
    }
    Q_ASSERT(*lastOffer == id);
    primarySelectionOffer.reset(lastOffer);
    lastOffer = nullptr;
    Q_EMIT q->selectionOffered(primarySelectionOffer.data());
}

DataControlDeviceV1::Private::Private(DataControlDeviceV1 *q)
    : q(q)
{
}

void DataControlDeviceV1::Private::setup(zwlr_data_control_device_v1 *d)
{
    Q_ASSERT(d);
    Q_ASSERT(!device.isValid());
    device.setup(d);
    zwlr_data_control_device_v1_add_listener(device, &s_listener, this);
}

DataControlDeviceV1::DataControlDeviceV1(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
}

DataControlDeviceV1::~DataControlDeviceV1()
{
    release();
}

void DataControlDeviceV1::destroy()
{
    d->device.destroy();
}

void DataControlDeviceV1::release()
{
    d->device.release();
}

bool DataControlDeviceV1::isValid() const
{
    return d->device.isValid();
}

void DataControlDeviceV1::setup(zwlr_data_control_device_v1 *dataDevice)
{
    d->setup(dataDevice);
}

void DataControlDeviceV1::setSelection(quint32 serial)
{
    Q_UNUSED(serial)
    zwlr_data_control_device_v1_set_selection(d->device, nullptr);
}

void DataControlDeviceV1::clearSelection(quint32 serial)
{
    setSelection(serial);
}

DataControlOfferV1 *DataControlDeviceV1::offeredSelection() const
{
    return d->selectionOffer.data();
}

DataControlOfferV1 *DataControlDeviceV1::primaryOfferedSelection() const
{
    return d->primarySelectionOffer.data();
}

DataControlDeviceV1::operator zwlr_data_control_device_v1*()
{
    return d->device;
}

DataControlDeviceV1::operator zwlr_data_control_device_v1*() const
{
    return d->device;
}

}
}
