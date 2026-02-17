#ifndef BASEMONITOR_H
#define BASEMONITOR_H
#include "uosai_global.h"

#include <QObject>
#include <QThread>
#include <QString>

namespace  uos_ai {

class BaseMonitor : public QThread
{
public:
    static BaseMonitor *instance();
    virtual QString getCurApp() { return ""; }
    virtual int getCurPid() { return 0; }
    virtual QString getAppByPid(int pid) { return ""; }

protected:
    explicit BaseMonitor(QObject *parent = nullptr);
Q_SIGNALS:
    virtual void mousePress(int x, int y) = 0;
    virtual void mouseRelease(int x, int y) = 0;
    virtual void keyEscapePress() = 0;
};
}

#endif // BASEMONITOR_H
