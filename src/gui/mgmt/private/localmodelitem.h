#ifndef LOCALMODELITEM_H
#define LOCALMODELITEM_H

#include <DWidget>
#include <DBackgroundGroup>
#include <DLabel>
#include <DSuggestButton>

namespace uos_ai {

class LocalModelItem: public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit LocalModelItem(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    ~LocalModelItem();

private:
    void initUI();
    void initConnect();
    void beginTimer(int time);
    void changeInstallStatus();
    void adjustSummaryLabelWidth();
    QString getUpdateVersion(const QByteArray &reply);

public:
    void setText(const QString &theme, const QString &summary);
    void setAppName(const QString &appName);
    void checkInstallStatus();
    void checkUpdateStatus();
    void checkStatusOntime();
    void stopTimer();
    bool getInstallStatus();
    void saveUpdateVersion();

signals:
    void changeUpdateStatus(bool);
    void sigRedPointVisible(bool);

private slots:
    void onInstall();
    void onUpdate();
    void onUninstall();
    void onChangeUpdateStatus(bool);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    DTK_WIDGET_NAMESPACE::DLabel *m_pLabelTheme = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pLabelSummary = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_redPoint = nullptr;
    DTK_WIDGET_NAMESPACE::DSuggestButton *m_pBtnInstall = nullptr;
    DTK_WIDGET_NAMESPACE::DSuggestButton *m_pBtnUpdate = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_pBtnUninstall = nullptr;

    QString m_appName;
    bool m_isInstall = false;
    int m_timerCount = 0;
    QTimer *m_timer = nullptr;
    QString m_updateVersion = "";
};
}

#endif // LOCALMODELITEM_H
