#ifndef DATA_CONTROL_DEVICE_H
#define DATA_CONTROL_DEVICE_H

#include <QObject>
#include <wayland-client.h>
#include "wlr-data-control-unstable-v1-client-protocol.h"

namespace uos_ai {
namespace wayland {

class DataControlOfferV1;

class DataControlDeviceV1 : public QObject
{
    Q_OBJECT
public:
    explicit DataControlDeviceV1(QObject *parent = nullptr);
    virtual ~DataControlDeviceV1();

    void setup(zwlr_data_control_device_v1 *dataDevice);
    void release();
    void destroy();
    bool isValid() const;

    void setSelection(quint32 serial);
    void clearSelection(quint32 serial);

    DataControlOfferV1 *offeredSelection() const;
    DataControlOfferV1 *primaryOfferedSelection() const;

    operator zwlr_data_control_device_v1*();
    operator zwlr_data_control_device_v1*() const;

Q_SIGNALS:
    void selectionOffered(uos_ai::wayland::DataControlOfferV1*);
    void dataOffered(uos_ai::wayland::DataControlOfferV1*);
    void dataControlOffered(uos_ai::wayland::DataControlOfferV1*);
    void selectionCleared();

private:
    class Private;
    QScopedPointer<Private> d;
};

}
}

#endif
