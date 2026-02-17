#ifndef XEVENTMONITOR_H
#define XEVENTMONITOR_H
#include "uosai_global.h"

#include "basemonitor.h"
#include <QDataStream>
#include <QTextStream>
#include <QObject>

// Virtual button codes that are not defined by X11.
#define Button1			1
#define Button2			2
#define Button3			3
#define WheelUp			4
#define WheelDown		5
#define WheelLeft		6
#define WheelRight		7
#define XButton1		8
#define XButton2		9

namespace  uos_ai {

class xEventMonitorPrivate;
class xEventMonitor : public BaseMonitor
{
    Q_OBJECT
    friend class xEventMonitorPrivate;
public:
    xEventMonitor(QObject *parent = 0);
    ~xEventMonitor() override;
    QString getCurApp() override { return m_curApp; }
    int getCurPid() override { return m_curPid; }
    QString getAppByPid(int pid) override;
Q_SIGNALS:
    void mousePress(int x, int y) override;
    void mouseRelease(int x, int y) override;
    void keyEscapePress() override;

protected:
    bool isWheelEvent(int detail);
    void run();

private:
    bool isOpenDisplay = false;
    xEventMonitorPrivate *d;
    QString m_curApp;
    int m_curPid;
};
}

#endif // XEVENTMONITOR_H
