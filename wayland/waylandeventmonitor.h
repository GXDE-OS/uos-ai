#ifndef WAYLANDEVENTMONITOR_H
#define WAYLANDEVENTMONITOR_H
#include "uosai_global.h"

#include "private/basemonitor.h"
#include <DRegionMonitor>

namespace  uos_ai {
class WaylandEventMonitor : public BaseMonitor
{
    Q_OBJECT
public:
    WaylandEventMonitor(QObject *parent = nullptr);
Q_SIGNALS:
    void mousePress(int x, int y) override;
    void mouseRelease(int x, int y) override;
    void keyEscapePress() override;

private:
    DTK_GUI_NAMESPACE::DRegionMonitor *m_reg = nullptr;
};
}

#endif // WAYLANDEVENTMONITOR_H
