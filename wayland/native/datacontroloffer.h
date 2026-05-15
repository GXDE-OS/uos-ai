#ifndef DATA_CONTROL_OFFER_H
#define DATA_CONTROL_OFFER_H

#include <QObject>
#include <QStringList>
#include <wayland-client.h>
#include "wlr-data-control-unstable-v1-client-protocol.h"

namespace uos_ai {
namespace wayland {

class DataControlDeviceV1;

class DataControlOfferV1 : public QObject
{
    Q_OBJECT
public:
    virtual ~DataControlOfferV1();

    void release();
    void destroy();
    bool isValid() const;

    QList<QString> offeredMimeTypes() const;

    void receive(const QString &mimeType, qint32 fd);

    operator zwlr_data_control_offer_v1*();
    operator zwlr_data_control_offer_v1*() const;

Q_SIGNALS:
    void mimeTypeOffered(const QString&);

private:
    friend class DataControlDeviceV1;
    explicit DataControlOfferV1(DataControlDeviceV1 *parent, zwlr_data_control_offer_v1 *dataOffer);
    class Private;
    QScopedPointer<Private> d;
};

}
}

Q_DECLARE_METATYPE(uos_ai::wayland::DataControlOfferV1*)

#endif
