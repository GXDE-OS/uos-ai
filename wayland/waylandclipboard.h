#ifndef WAYLANDCLIPBOARD_H
#define WAYLANDCLIPBOARD_H


#include <QGuiApplication>
#include <QDebug>
#include <QObject>
#include <QClipboard>
#include <QtConcurrent/QtConcurrent>
#include <QThread>
#include <QThreadPool>
#include <QFile>
#include <cerrno>
#include <cstring>

#include "native/connection_thread.h"
#include "native/event_queue.h"
#include "native/registry.h"
#include "native/seat.h"
#include "native/datacontroldevicemanager.h"
#include "native/datacontroldevice.h"
#include "native/datacontroloffer.h"

#include "private/baseclipboard.h"
#include <unistd.h>
#include <QLoggingCategory>

using namespace uos_ai::wayland; 

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
