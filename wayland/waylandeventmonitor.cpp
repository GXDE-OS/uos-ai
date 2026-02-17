#include "waylandeventmonitor.h"
#include <DRegionMonitor>
#include <QDebug>
#include <QPoint>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE
DGUI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logWayland)

WaylandEventMonitor::WaylandEventMonitor(QObject *parent) : BaseMonitor(parent)
{
    m_reg = new DRegionMonitor(this);
    connect(m_reg, &DRegionMonitor::buttonPress, this, [=](const QPoint &pos, int button){
        Q_EMIT mousePress(pos.x(), pos.y());
    });

    connect(m_reg, &DRegionMonitor::buttonRelease, this, [=](const QPoint &pos, int button){
        Q_EMIT mouseRelease(pos.x(), pos.y());
    });

    m_reg->setCoordinateType(DRegionMonitor::Original);
    m_reg->registerRegion();
}

