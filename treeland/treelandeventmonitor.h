#ifndef TREELANDEVENTMONITOR_H
#define TREELANDEVENTMONITOR_H
#include "uosai_global.h"

#include "private/basemonitor.h"
#include "treelandddeactivev1.h"
#include <QObject>

#include <QThread>

namespace  uos_ai {

class TreelandEventMonitor : public BaseMonitor
{
    Q_OBJECT
public:
    TreelandEventMonitor(QObject *parent = nullptr);
Q_SIGNALS:
    void mousePress(int x, int y) override;
    void mouseRelease(int x, int y) override;
    void keyEscapePress() override;

private:
    std::unique_ptr<TreelandDDEShellManageV1> m_manager;
    std::unique_ptr<TreelandDDEActiveV1> m_active;
};
}

#endif // TREELANDEVENTMONITOR_H
