#ifndef LOCALMODELLISTITEM_H
#define LOCALMODELLISTITEM_H

#include "iconbuttonex.h"

#include <DWidget>
#include <DBackgroundGroup>
#include <DLabel>
#include <DSwitchButton>

#include <QProcess>

DWIDGET_USE_NAMESPACE

class LocalModelListItem: public DWidget
{
    Q_OBJECT

public:
    explicit LocalModelListItem(DWidget *parent = nullptr);
    ~LocalModelListItem();

private:
    void initUI();
    void initConnect();

public:
    void setText(const QString &theme, const QString &summary);
    void setSwitchChecked(bool);
    void setAppName(const QString &appName);

signals:
    void signalUninstall();
    void signalSwitchChanged(bool);

private slots:
    void onUninstall();

private:
    DLabel *m_pLabelTheme = nullptr;
    DLabel *m_pLabelSummary = nullptr;
    IconButtonEx *m_pBtnUninstall = nullptr;
    DSwitchButton *m_pBtnSwitch = nullptr;

    QProcess *m_pProcess = nullptr;
    QString m_appName;
};

#endif // LOCALMODELLISTITEM_H
