#ifndef MCPSERVERWIDGET_H
#define MCPSERVERWIDGET_H

#include "uosai_global.h"

#include <DWidget>
#include <DBackgroundGroup>

#include <QProcess>
#include <QJsonArray>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

class ThemedLable;
namespace uos_ai {
class McpServerItem;
class McpServerListWidget;
class McpServerWidget: public DWidget
{
    Q_OBJECT

public:
    explicit McpServerWidget(DWidget *parent = nullptr);
    ~McpServerWidget();
    QString getTitleName();
    void updateStatus();
    bool getIsInstalled();

public Q_SLOTS:
    void beginTimer();
    void stopTimer();

private slots:
    void onThemeTypeChanged();
    void checkStatusOntime();

private:
    void initUI();
    void changeInstallStatus();

    DBackgroundGroup *serverWidget();

private:
    QVBoxLayout *m_mainLayout = nullptr;
    ThemedLable *m_pWidgetLabel = nullptr;
    ThemedLable *m_pEnvWidgetLabel = nullptr;
    DBackgroundGroup *m_pServerWidget = nullptr;
    McpServerItem *m_pServerItem = nullptr;
    McpServerListWidget *m_pServersListWidget = nullptr;

    QProcess *m_pProcess = nullptr;
    QTimer *m_timer = nullptr;
    int m_timerCount = 0;
    bool m_isInstalled = false;
signals:
    void sigThirdPartyMcpAgree();
    void sigAgentInstallChanged(bool isInstalled);

};
}

#endif // MCPSERVERWIDGET_H
