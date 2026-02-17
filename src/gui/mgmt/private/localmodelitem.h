#ifndef LOCALMODELITEM_H
#define LOCALMODELITEM_H

#include <QProcess>

#include <DWidget>
#include <DBackgroundGroup>
#include <DLabel>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

class LocalModelItem: public DWidget
{
    Q_OBJECT

public:
    explicit LocalModelItem(DWidget *parent = nullptr);
    ~LocalModelItem();

private:
    void initUI();
    void initConnect();
    void beginTimer(const int &time);
    void addLocalLlM();
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
    DLabel *m_pLabelTheme = nullptr;
    DLabel *m_pLabelSummary = nullptr;
    DLabel *m_redPoint = nullptr;
    DSuggestButton *m_pBtnInstall = nullptr;
    DSuggestButton *m_pBtnUpdate = nullptr;
    DPushButton *m_pBtnUninstall = nullptr;

    QProcess *m_pProcess = nullptr;
    QString m_appName;
    bool m_isInstall = false;
    int m_timerCount = 0;
    QTimer *m_timer = nullptr;
    QString m_updateVersion = "";
};

#endif // LOCALMODELITEM_H
