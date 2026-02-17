#include "basemonitor.h"

#ifdef COMPILE_ON_V25
#include "treelandeventmonitor.h"
#endif
#ifdef COMPILE_ON_V20
#include "waylandeventmonitor.h"
#endif
#include "esystemcontext.h"
#include "xeventmonitor.h"

using namespace uos_ai;

BaseMonitor::BaseMonitor(QObject *parent) : QThread(parent)
{

}

BaseMonitor *BaseMonitor::instance()
{
#ifdef COMPILE_ON_V25
    if (ESystemContext::isWayland()) {
        static TreelandEventMonitor instance;
        return &instance;
    }
#endif
#ifdef COMPILE_ON_V20
    if (ESystemContext::isWayland()) {
        static WaylandEventMonitor instance;
        return &instance;
    }
#endif
    static xEventMonitor instance;
    return &instance;
}
