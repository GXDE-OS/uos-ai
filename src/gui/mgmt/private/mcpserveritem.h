#ifndef MCPSERVERITEM_H
#define MCPSERVERITEM_H

#include <DWidget>
#include <DSwitchButton>
#include <DLabel>
#include <DSuggestButton>
#include <DPushButton>

#include <QProcess>
#include <QTimer>

namespace uos_ai {
class McpServerItem : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT
public:
    explicit McpServerItem(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
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
    DTK_WIDGET_NAMESPACE::DLabel *m_pNameLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pDescLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DSuggestButton *m_pBtnInstall = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_pBtnUninstall = nullptr;
    DTK_WIDGET_NAMESPACE::DSuggestButton *m_pBtnUpdate = nullptr;

    QProcess *m_pProcess = nullptr;

    QString m_appName = "uos-ai-agent";  // MCP服务的包名
    QString m_updateVersion;
};
}

#endif // MCPSERVERITEM_H
