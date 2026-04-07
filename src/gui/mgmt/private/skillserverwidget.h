#ifndef SKILLSERVERWIDGET_H
#define SKILLSERVERWIDGET_H

#include "uosai_global.h"

#include <DWidget>
#include <DBackgroundGroup>

#include <QJsonArray>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

class ThemedLable;
namespace uos_ai {
class SkillServerItem;
class SkillServerListWidget;
class SkillServerWidget: public DWidget
{
    Q_OBJECT

public:
    explicit SkillServerWidget(DWidget *parent = nullptr);
    ~SkillServerWidget();
    QString getTitleName();
    void updateStatus();

public Q_SLOTS:
    void changeInstallStatus(bool isInstall);

private slots:
    void onThemeTypeChanged();

private:
    void initUI();

    DBackgroundGroup *serverWidget();

private:
    QVBoxLayout *m_mainLayout = nullptr;
    ThemedLable *m_pWidgetLabel = nullptr;
    ThemedLable *m_pEnvWidgetLabel = nullptr;
    DBackgroundGroup *m_pServerWidget = nullptr;
    SkillServerItem *m_pServerItem = nullptr;
    SkillServerListWidget *m_pServersListWidget = nullptr;

    bool m_isInstalled = false;
signals:
    void sigThirdPartyMcpAgree();
    void sigNavigateToMcpServerPage();

};
}

#endif // SKILLSERVERWIDGET_H
