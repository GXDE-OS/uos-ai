#ifndef MCPSERVERWIDGET_H
#define MCPSERVERWIDGET_H

#include <DWidget>
#include <DBackgroundGroup>

#include <QProcess>
#include <QJsonArray>
#include <QVBoxLayout>

namespace uos_ai {

class ThemedLable;
class McpServerItem;
class McpServerListWidget;
class McpServerWidget : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit McpServerWidget(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    ~McpServerWidget();
    QString getTitleName();
    void updateStatus();

public Q_SLOTS:
    void beginTimer();
    void stopTimer();

private slots:
    void onThemeTypeChanged();
    void checkStatusOntime();

private:
    void initUI();
    void changeInstallStatus();

    DTK_WIDGET_NAMESPACE::DBackgroundGroup *serverWidget();

private:
    QVBoxLayout *m_mainLayout = nullptr;
    ThemedLable *m_pWidgetLabel = nullptr;
    ThemedLable *m_pEnvWidgetLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *m_pServerWidget = nullptr;
    McpServerItem *m_pServerItem = nullptr;
#if 0
    McpServerListWidget *m_pServersListWidget = nullptr;
#endif

    QTimer *m_timer = nullptr;
    int m_timerCount = 0;
    bool m_isInstalled = false;
signals:
    void sigThirdPartyMcpAgree();

};
}

#endif // MCPSERVERWIDGET_H
