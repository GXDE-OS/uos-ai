#include "treelandddeactivev1.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logTreeland)

TreelandDDEShellManageV1::TreelandDDEShellManageV1()
    : QWaylandClientExtensionTemplate<TreelandDDEShellManageV1>(TREELANDDDESHELLMANAGERV1VERSION)
    , QtWayland::treeland_dde_shell_manager_v1()
{
}

void TreelandDDEShellManageV1::instantiate()
{
    initialize();
}

TreelandDDEActiveV1::TreelandDDEActiveV1(struct ::treeland_dde_active_v1 *id)
    : QtWayland::treeland_dde_active_v1(id)
{
}

TreelandDDEActiveV1::~TreelandDDEActiveV1()
{
    destroyed();
}

void TreelandDDEActiveV1::treeland_dde_active_v1_active_in(uint32_t reason)
{
    qCDebug(logTreeland) << "Active in event received, reason:" << reason;
    Q_EMIT mousePress();
}

void TreelandDDEActiveV1::treeland_dde_active_v1_active_out(uint32_t reason)
{
    qCDebug(logTreeland) << "Active out event received, reason:" << reason;
    Q_EMIT mouseRelease();
}

void TreelandDDEActiveV1::treeland_dde_active_v1_start_drag()
{
    qWarning() << "------------start_drag";
}

void TreelandDDEActiveV1::treeland_dde_active_v1_drop()
{
    qWarning() << "------------drop";
}
