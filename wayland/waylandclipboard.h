#ifndef WAYLANDCLIPBOARD_H
#define WAYLANDCLIPBOARD_H
#include "uosai_global.h"

#include <QGuiApplication>
#include <QDebug>
#include <QObject>
#include <QClipboard>
#include <QtConcurrent/QtConcurrent>

#include "KF5/KWayland/Client/compositor.h"
#include "KF5/KWayland/Client/connection_thread.h"
#include "KF5/KWayland/Client/event_queue.h"
#include "KF5/KWayland/Client/registry.h"
#include "KF5/KWayland/Client/seat.h"

#include "KF5/KWayland/Client/datacontroldevicemanager.h"
#include "KF5/KWayland/Client/datacontroldevice.h"
#include "KF5/KWayland/Client/datacontrolsource.h"
#include "KF5/KWayland/Client/datacontroloffer.h"

#include "private/baseclipboard.h"
#include <unistd.h>

using namespace KWayland::Client; 

namespace uos_ai {
class WaylandClipboard : public BaseClipboard
{
    Q_OBJECT
public:
    explicit WaylandClipboard(QObject *parent = nullptr);
    virtual ~WaylandClipboard();
    QString getClipText() override;
    void clearClipText() override;
    void setClipText(const QString &text) override;
    bool isScribeWordsVisible() override;
    void blockChangedSignal(bool) override;

private:
    void init();
    void setupRegistry(Registry *registry);

private:
    QThread *m_connectionThread;
    ConnectionThread *m_connectionThreadObject;
    EventQueue *m_eventQueue;

    DataControlDeviceManager *m_dataControlDeviceManager;
    DataControlDeviceV1 *m_dataControlDevice;
    DataControlOfferV1 *m_copyControlOffer;

    Seat *m_seat;
};
}

#endif // WAYLANDCLIPBOARD_H
