#include "treelandeventmonitor.h"

#include <QDebug>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logTreeland)

TreelandEventMonitor::TreelandEventMonitor(QObject *parent) : BaseMonitor(parent)
{
    m_manager.reset(new TreelandDDEShellManageV1);
    QObject::connect(m_manager.get(), &TreelandDDEShellManageV1::activeChanged, this, [this](){
        if (m_manager->isActive()) {
            qCDebug(logTreeland) << "DDEShell manager activated";
            auto waylandApp = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
            if (!waylandApp) {
                qCWarning(logTreeland) << "Failed to get wayland application interface";
                return;
            }
            auto seat = waylandApp->seat();

            if (!seat) {
                qCCritical(logTreeland) << "Failed to get wl_seat from QtWayland QPA";
                qFatal("Failed to get wl_seat from QtWayland QPA!");
            }

            m_active.reset(new TreelandDDEActiveV1(m_manager->get_treeland_dde_active(seat)));
            qCDebug(logTreeland) << "Created new DDE active interface";
        }
    });

    m_manager->instantiate();
    qCDebug(logTreeland) << "Manager instantiated";

    QObject::connect(m_active.get(), &TreelandDDEActiveV1::mousePress, this, [this](){
        qCDebug(logTreeland) << "Mouse press event received";
        Q_EMIT mousePress(-1, -1);
    });

    QObject::connect(m_active.get(), &TreelandDDEActiveV1::mouseRelease, this, [this](){
        qCDebug(logTreeland) << "Mouse release event received";
        Q_EMIT mouseRelease(-1, -1);
    });
}

