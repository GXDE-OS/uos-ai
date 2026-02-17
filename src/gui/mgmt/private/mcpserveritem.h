#ifndef MCPSERVERITEM_H
#define MCPSERVERITEM_H

#include "uosai_global.h"

#include <DWidget>
#include <DSwitchButton>
#include <DLabel>
#include <DSuggestButton>
#include <DPushButton>

#include <QProcess>
#include <QTimer>

DWIDGET_USE_NAMESPACE

namespace uos_ai {
class McpServerItem: public DWidget
{
    Q_OBJECT
public:
    explicit McpServerItem(DWidget *parent = nullptr);
    ~McpServerItem() override;

    void setText(const QString &theme, const QString &summary);
    void setAppName(const QString &appName);
    void checkUpdateStatus(bool isInstalled);
    void changeInstallStatus(bool isInstalled);

Q_SIGNALS:
    void doCheckInstalled();

protected:
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    void onInstall();
    void onUninstall();
    void onUpdate();

private:
    void initUI();
    void initConnect();

    QString getUpdateVersion(const QByteArray& reply);
    void adjustSummaryLabelWidth();
private:
    DLabel *m_pNameLabel = nullptr;
    DLabel *m_pDescLabel = nullptr;
    DSuggestButton *m_pBtnInstall = nullptr;
    DPushButton *m_pBtnUninstall = nullptr;
    DSuggestButton *m_pBtnUpdate = nullptr;

    QProcess *m_pProcess = nullptr;

    QString m_appName = "uos-ai-agent";  // MCP服务的包名
    QString m_updateVersion;
};
}

#endif // MCPSERVERITEM_H
