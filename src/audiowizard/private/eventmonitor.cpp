#include <QDebug>
#include <qcursor.h>
#include <QGuiApplication>
#include <QScreen>
#include <QtMath>

#include <X11/Xlib.h>
#include <X11/extensions/record.h>
#include <X11/Xlibint.h>
#include "eventmonitor.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

#ifndef ENABLE_DBUS_XEVENT

UOSAI_BEGIN_NAMESPACE

class EventMonitorPrivate
{
public:
    Display* m_display = nullptr;
    static void callback(XPointer ptr, XRecordInterceptData* data);
    void handleXRecordEvent(XRecordInterceptData* data);
    EventMonitor *q;
};

UOSAI_END_NAMESPACE

UOSAI_USE_NAMESPACE

EventMonitor::EventMonitor(QObject* parent):QThread (parent)
  , m_releaseX(0)
  , m_releaseY(0)
{
    QPoint pt = QCursor::pos();
    m_releaseX = pt.x();
    m_releaseY = pt.y();
    d = new EventMonitorPrivate;
    d->q = this;
    this->start(QThread::LowestPriority);
}

EventMonitor* EventMonitor::instance()
{
    static EventMonitor event;
    return &event;
}

void EventMonitor::run()
{
    qCDebug(logAudioWizard) << "Starting event monitoring thread";
    d->m_display = XOpenDisplay(0);
    if(d->m_display == nullptr)
    {
        qCCritical(logAudioWizard) << "Failed to open X11 display";
        return;
    }

    XRecordClientSpec clients = XRecordAllClients;
    XRecordRange* range = XRecordAllocRange();
    if(range == nullptr)
    {
        qCCritical(logAudioWizard) << "Failed to allocate XRecordRange";
        return;
    }

    memset(range, 0, sizeof(XRecordRange));
    range->device_events.first = KeyPress;
    range->device_events.last = ButtonRelease;

//    range->device_events.first = ButtonPress;
//    range->device_events.last = ButtonRelease;

    XRecordContext context = XRecordCreateContext(d->m_display, 0, &clients, 1, &range, 1);
    if(context == 0)
    {
        qCCritical(logAudioWizard) << "Failed to create XRecordContext";
        return;
    }

    XFree(range);
    XSync(d->m_display, True);
    Display* display_datalink = XOpenDisplay(0);
    if(display_datalink == nullptr)
    {
        qCCritical(logAudioWizard) << "Failed to open data link display";
        return;
    }
    
    if(XRecordEnableContext(display_datalink, context, d->callback, (XPointer)d))
    {
        qCCritical(logAudioWizard) << "Failed to enable XRecord context";
        return;
    }

}

bool EventMonitor::keyPressed(int keycode)
{
    auto keyIter = m_keyPressedMap.find(keycode);
    if(keyIter != m_keyPressedMap.end())
    {
        if(keyIter.value())
        {
            return false;
        }
    }
    m_keyPressedMap[keycode] = true;
    return true;
}

bool EventMonitor::keyReleased(int keycode)
{
    auto keyIter = m_keyPressedMap.find(keycode);
    if(keyIter != m_keyPressedMap.end())
    {
        keyIter.value() = false;
    }
    return true;
}

void EventMonitor::getReleasePoint(int& x, int& y)
{
    double screenScale = QGuiApplication::primaryScreen()->devicePixelRatio();
    x = qCeil(m_releaseX / screenScale);
    y = qCeil(m_releaseY / screenScale);
}

void EventMonitorPrivate::callback(XPointer ptr, XRecordInterceptData* data)
{
    ((EventMonitorPrivate*)ptr)->handleXRecordEvent(data);
}

void EventMonitorPrivate::handleXRecordEvent(XRecordInterceptData* data)
{
    if(data->category == XRecordFromServer)
    {
        KeyCode keycode = data->data[1];
        xEvent* event = (xEvent*)data->data;
        switch (event->u.u.type) {
        case KeyPress:
            if(50 != keycode && 118 != keycode && 22 != keycode)//shift+insert backspace(22)
            {
                if(!q->keyPressed(keycode))
                {
                    return;
                }
                emit q->sendKeyPressEventsignal(EventType::EVENT_KEYPRESS);
            }
            break;
        case KeyRelease:
        {
            q->keyReleased(keycode);
        }
            break;
        case ButtonPress:
            Q_EMIT q->sendMouseClickedEventsignal(EventType::EVENT_BUTTONPRESS, event->u.keyButtonPointer.rootX, event->u.keyButtonPointer.rootY);
            break;
        case ButtonRelease:
            Q_EMIT q->sendMouseClickedEventsignal(EventType::EVENT_BUTTONRELEASE, event->u.keyButtonPointer.rootX, event->u.keyButtonPointer.rootY);
            q->m_releaseX = event->u.keyButtonPointer.rootX;
            q->m_releaseY = event->u.keyButtonPointer.rootY;
            break;
        default:
            break;
        }
    }
    fflush(stdout);
    XRecordFreeData(data);
}

#else
/////////////////////////////////////////////////////////////////////////////////////////
EventMonitor::EventMonitor(QObject* parent):QThread (parent)
{
    m_xeventMonitor = new com::deepin::api::XEventMonitor(
                    "com.deepin.api.XEventMonitor", "/com/deepin/api/XEventMonitor", QDBusConnection::sessionBus(), this);
        connect(m_xeventMonitor,&com::deepin::api::XEventMonitor::ButtonPress,this,&EventMonitor::onButtonPress);
        connect(m_xeventMonitor,&com::deepin::api::XEventMonitor::ButtonRelease,this,&EventMonitor::onButtonRelease);
        connect(m_xeventMonitor,&com::deepin::api::XEventMonitor::KeyPress,this,&EventMonitor::onKeyPress);
        connect(m_xeventMonitor,&com::deepin::api::XEventMonitor::KeyRelease,this,&EventMonitor::onKeyRelease);
}
EventMonitor* EventMonitor::instance()
{
    static EventMonitor instanceObj;
    return &instanceObj;
}
void EventMonitor::onButtonPress(int key,int x,int y,QString uid)
{
    Q_UNUSED(key);
    Q_UNUSED(uid);
    Q_UNUSED(x);
    Q_UNUSED(y);
    emit sendMouseClickedEventsignal(EVENT_BUTTONPRESS, x, y);
}
void EventMonitor::onButtonRelease(int key,int x,int y,QString uid)
{
    Q_UNUSED(key);
    Q_UNUSED(uid);
    m_releaseX = x;
    m_releaseY = y;
    emit sendMouseClickedEventsignal(EVENT_BUTTONRELEASE, x, y);
}
void EventMonitor::onKeyPress(QString str,int cursorX,int cursorY,QString uid)
{
    Q_UNUSED(str);
    Q_UNUSED(uid);
    Q_UNUSED(cursorX);
    Q_UNUSED(cursorY);
    emit sendKeyPressEventsignal(EventType::EVENT_KEYPRESS);
}
void EventMonitor::onKeyRelease(QString str,int cursorX,int cursorY,QString uid)
{
    Q_UNUSED(str);
    Q_UNUSED(uid);
    Q_UNUSED(cursorX);
    Q_UNUSED(cursorY);
}
void EventMonitor::getReleasePoint(int& x, int& y)
{
    x = m_releaseX;
    y = m_releaseY;
}

#endif
