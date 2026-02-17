#ifndef EVENTMONITOR_H
#define EVENTMONITOR_H
//#define ENABLE_DBUS_XEVENT
#ifndef  ENABLE_DBUS_XEVENT

#include "uosai_global.h"
#include <QThread>
#include <QMap>

namespace uos_ai {

enum EventType{
    EVENT_BUTTONPRESS = 0,
    EVENT_BUTTONRELEASE =1,
    EVENT_KEYPRESS = 2
};

class EventMonitorPrivate;
class EventMonitor : public QThread
{
    Q_OBJECT
public:
    static EventMonitor* instance();
    EventMonitor(QObject* parent = nullptr);
    void getReleasePoint(int& x, int& y);
    bool keyPressed(int keycode);
    bool keyReleased(int keycode);
    int m_releaseX;
    int m_releaseY;

Q_SIGNALS:
    void sendMouseClickedEventsignal(int type, int x, int y);
    void sendKeyPressEventsignal(int type);
private:
    void run();
private:
    QMap<int,bool> m_keyPressedMap;

    EventMonitorPrivate *d = nullptr;
};
}
#else
#include <QThread>
#include <com_deepin_api_xeventmonitor.h>
#include <QMap>
#include "ui/global.h"
class EventMonitor : public QThread
{
    Q_OBJECT
public:
    enum MouseKeyType{MouseLeft = 1,MouseMiddle = 2, MouseRight = 3};
    static EventMonitor* instance();
    EventMonitor(QObject* parent = nullptr);
    void getReleasePoint(int& x, int& y);
public slots:
    void onButtonPress(int key,int x,int y,QString uid);
    void onButtonRelease(int key,int x,int y,QString uid);
    void onKeyPress(QString str,int cursorX,int cursorY,QString uid);
    void onKeyRelease(QString str,int cursorX,int cursorY,QString uid);
signals:
    void sendMouseClickedEventsignal(int type, int x, int y);
    void sendKeyPressEventsignal(int type);

private:
    int m_releaseX;
    int m_releaseY;

    QMap<int,bool> m_keyPressedMap;
    com::deepin::api::XEventMonitor* m_xeventMonitor;
};

#endif
#endif // EVENTMONITOR_H

