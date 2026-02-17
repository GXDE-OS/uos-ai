#include "xeventmonitor.h"

#include <QDebug>
#include <QFile>

#include <X11/Xlib.h>
#include <X11/extensions/record.h>
#include <X11/Xlibint.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logWordWizard)

#define MAX_OPENDISPLAY_COUNT 10

UOSAI_BEGIN_NAMESPACE

class xEventMonitorPrivate
{
public:
    static void callback(XPointer trash, XRecordInterceptData* data);
    void handleRecordEvent(XRecordInterceptData *);
    XRecordContext m_context;
    Display *m_displayDatalink = nullptr;
    Display *keyPressDisplay = nullptr;
    Display *windowInfoDisplay = nullptr;

    int preessedOn = -1;
    xEventMonitor *q;
    
    // 获取指定位置的窗口ID
    QPair<int, Window> getWindowAtCursor();
    Window getSubwindow(Window window);
    QPair<int, Window> scanWindowPid(Window window);
    // 获取窗口的PID
    int getWindowPid(Window window);
    // 获取进程名称
    QString getProcessName(int pid);
    // 获取Window名称
    QString getWindowName(Window window);
};

UOSAI_END_NAMESPACE

UOSAI_USE_NAMESPACE

xEventMonitor::xEventMonitor(QObject *parent) : BaseMonitor(parent)
{
    d = new xEventMonitorPrivate;
    d->q = this;
    start(QThread::LowestPriority);
}

xEventMonitor::~xEventMonitor()
{
    if (d) {
        if (d->windowInfoDisplay) {
            XCloseDisplay(d->windowInfoDisplay);
            d->windowInfoDisplay = nullptr;
        }
        
        if (d->keyPressDisplay) {
            XCloseDisplay(d->keyPressDisplay);
            d->keyPressDisplay = nullptr;
        }
        
        delete d;
        d = nullptr;
    }
}

void xEventMonitor::run()
{
    int count = 0;
    while (!isOpenDisplay) {
        if(MAX_OPENDISPLAY_COUNT <= count) {
            qCInfo(logWordWizard) << "unable to open display and opendisplay is 10 timesd\n";
            break;
        }

        msleep(500 * count);
        ++count;

        // Receive from ALL clients, including future clients.
        XRecordClientSpec clients = XRecordAllClients;
        XRecordRange* range = XRecordAllocRange();
        if (range == nullptr) {
            qCWarning(logWordWizard) <<"unable to allocate XRecordRange\n";
            continue;
        }

        // Receive KeyPress, KeyRelease, ButtonPress, ButtonRelease and MotionNotify events.
        memset(range, 0, sizeof(XRecordRange));
        range->device_events.first = KeyPress;
        range->device_events.last  = ButtonRelease;

        d->windowInfoDisplay = XOpenDisplay(nullptr);
        d->m_displayDatalink = XOpenDisplay(nullptr);

        if (d->m_displayDatalink == nullptr) {
            qCWarning(logWordWizard) << "unable to open second display\n";
            XFree(range);
            continue;
        }

        // And create the XRECORD context.
        d->m_context = XRecordCreateContext(d->m_displayDatalink, 0, &clients, 1, &range, 1);
        if (d->m_context == 0) {
            qCWarning(logWordWizard) <<"XRecordCreateContext failed\n";
            XFree(range);
            XCloseDisplay(d->m_displayDatalink);
            continue;
        }

        if (!XRecordEnableContext(d->m_displayDatalink, d->m_context, d->callback, (XPointer) this)) {
            qCWarning(logWordWizard) << "XRecordEnableContext() failed\n";
            XRecordFreeContext(d->m_displayDatalink, d->m_context);
            XFree(range);
            XCloseDisplay(d->m_displayDatalink);
            continue;
        }

        isOpenDisplay = true;
        XRecordFreeContext(d->m_displayDatalink, d->m_context);
        XFree(range);
        XCloseDisplay(d->m_displayDatalink);
        XCloseDisplay(d->keyPressDisplay);
    }
    qCInfo(logWordWizard) << "return";
    return;
}

void xEventMonitorPrivate::callback(XPointer ptr, XRecordInterceptData *data)
{
    ((xEventMonitor *) ptr)->d->handleRecordEvent(data);
    return;
}

void xEventMonitorPrivate::handleRecordEvent(XRecordInterceptData *data)
{
    if (data->category == XRecordFromServer) {
        xEvent *event = (xEvent *)data->data;
        switch (event->u.u.type) {
        case ButtonPress:
            q->m_curApp = getWindowName(getSubwindow(DefaultRootWindow(windowInfoDisplay)));
            q->m_curPid = scanWindowPid(getSubwindow(DefaultRootWindow(windowInfoDisplay))).first;
            //qCInfo(logWordWizard) << "cursor window name:" << q->m_curApp;
            emit q->mousePress(event->u.keyButtonPointer.rootX, event->u.keyButtonPointer.rootY);
            break;
        case ButtonRelease: {
            emit q->mouseRelease(event->u.keyButtonPointer.rootX, event->u.keyButtonPointer.rootY);
            break;
        }
        case KeyPress: {
            if (!keyPressDisplay) {
                keyPressDisplay = XOpenDisplay(nullptr);
                if (!keyPressDisplay) {
                    qCWarning(logWordWizard) << "unable to open keyPress display\n";
                    break;
                }
            }
            KeySym key = XKeycodeToKeysym(keyPressDisplay, event->u.u.detail, 0);
            if (key == XK_Escape)
                emit q->keyEscapePress();
            break;
        }
        default:
            break;
        }
    }
    XRecordFreeData(data);
    return;
}

QPair<int, Window> xEventMonitorPrivate::getWindowAtCursor()
{
    if (!windowInfoDisplay)
        return {-1, None};

    return scanWindowPid(getSubwindow(DefaultRootWindow(windowInfoDisplay)));
}

Window xEventMonitorPrivate::getSubwindow(Window window)
{
    if (!windowInfoDisplay)
        return None;

    Window rootWindow = DefaultRootWindow(windowInfoDisplay);
    Window childWindow = None;
    int rootX, rootY, winX, winY;
    unsigned int mask;
    XQueryPointer(windowInfoDisplay, window, &rootWindow, &childWindow,
                      &rootX, &rootY, &winX, &winY, &mask);
    return childWindow;
}

QPair<int, Window> xEventMonitorPrivate::scanWindowPid(Window window)
{
    if (window == None)
        return qMakePair(-1, None);

    int pid = getWindowPid(window);
    if (pid < 0) {
        return scanWindowPid(getSubwindow(window));
    }
    return qMakePair(pid, window);
}

int xEventMonitorPrivate::getWindowPid(Window window)
{
    if (!windowInfoDisplay || window == None)
        return -1;
        
    Atom actualType;
    int actualFormat;
    unsigned long numItems, bytesAfter;
    unsigned char *propData = nullptr;
    int pid = -1;
    
    Atom pidAtom = XInternAtom(windowInfoDisplay, "_NET_WM_PID", False);
    
    if (XGetWindowProperty(windowInfoDisplay, window, pidAtom, 0, 1, False, XA_CARDINAL,
                          &actualType, &actualFormat, &numItems, &bytesAfter, &propData) == Success) {
        if (propData) {
            if (actualType == XA_CARDINAL && actualFormat == 32 && numItems > 0) {
                pid = *(long*)propData;
            }
            XFree(propData);
        }
    }
    
    return pid;
}

QString xEventMonitorPrivate::getProcessName(int pid)
{
    if (pid <= 0)
        return "";
        
    QString processName;
    QString cmdlinePath = QString("/proc/%1/cmdline").arg(pid);
    QFile file(cmdlinePath);
    
    if (file.open(QFile::ReadOnly)) {
        processName = QString(file.readLine());
        file.close();
        
        // 简单处理命令行，获取可执行文件名
        QStringList parts = processName.split('/');
        if (!parts.isEmpty()) {
            processName = parts.last().split(' ').first();
        }
    }
    
    return processName;
}

QString xEventMonitorPrivate::getWindowName(Window window)
{
    if (!windowInfoDisplay || window == None)
        return "";

    QString windowName;
    Atom actualType;
    int actualFormat;
    unsigned long numItems, bytesAfter;
    unsigned char *propData = nullptr;

    // 获取 WM_CLASS 属性
    Atom prop = XInternAtom(windowInfoDisplay, "WM_CLASS", False);

    if (XGetWindowProperty(windowInfoDisplay, window, prop, 0, 1024, False, AnyPropertyType,
                           &actualType, &actualFormat, &numItems, &bytesAfter, &propData) == Success) {
        if (propData) {
            char *res_class = (char *)propData;
            char *res_name = res_class + strlen(res_class) + 1;
            //printf("Class: %s, Name: %s\n", res_class, res_name);
            windowName = res_class;
            XFree(propData);
        }
    }

    if (windowName.isEmpty()) {
        return getWindowName(getSubwindow(window));
    }

    return windowName;
}

QString xEventMonitor::getAppByPid(int pid) {
    if (pid == m_curPid) {
        return m_curApp;
    }

    // 通过PID获取进程名
    QString processName = d->getProcessName(pid);
    if (!processName.isEmpty()) {
        return processName;
    }

    // 尝试通过窗口获取应用名
    QPair<int, Window> windowInfo = d->getWindowAtCursor();
    if (windowInfo.first == pid) {
        return d->getWindowName(windowInfo.second);
    }

    return QString();
}

bool xEventMonitor::isWheelEvent(int detail)
{
    //滚轮判断
    return !(detail != WheelUp && detail != WheelDown && detail != WheelLeft && detail != WheelRight);
}

