#ifndef SKILLSERVERITEM_H
#define SKILLSERVERITEM_H

#include "uosai_global.h"

#include <DWidget>
#include <DSwitchButton>
#include <DLabel>
#include <DSuggestButton>
#include <DPushButton>

#include <QTimer>

DWIDGET_USE_NAMESPACE

namespace uos_ai {
class SkillServerItem: public DWidget
{
    Q_OBJECT
public:
    explicit SkillServerItem(DWidget *parent = nullptr);
    ~SkillServerItem() override;

    void setText(const QString &theme, const QString &target, const QString &summary);
    void checkUpdateStatus(bool isInstalled);
    void changeInstallStatus(bool isInstalled);

Q_SIGNALS:
    void doCheckInstalled();
    void sigNavigateToMcpServer();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void initUI();
    void adjustSummaryLabelWidth();
private:
    DLabel *m_pNameLabel = nullptr;
    DLabel *m_pDescLabel = nullptr;
};
}

#endif // SKILLSERVERITEM_H
