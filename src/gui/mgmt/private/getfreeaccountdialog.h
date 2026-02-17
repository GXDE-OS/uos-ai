#ifndef GETFREEACCOUNTDIALOG_H
#define GETFREEACCOUNTDIALOG_H

#include <DDialog>
#include <DWidget>
#include <DCheckBox>
#include <DIconButton>
#include <DArrowRectangle>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>
#include <DGuiApplicationHelper>

#include <QFutureWatcher>
#include <QtConcurrent>

#include "tasdef.h"
#include "networkdefs.h"

DWIDGET_USE_NAMESPACE

class LLMServerProxy;
class WrapCheckBox;
class ThemedLable;

class GetFreeAccountDialog : public DDialog
{
    Q_OBJECT
public:
    explicit GetFreeAccountDialog(DWidget *parent = nullptr);

    bool isFreeAccount();
    void resetDialog();

private:
    void initUI();
    void initConnect();
    void resetLinkColor();

signals:
    void signalAppendModel(const LLMServerProxy &);
    void signalActivityEnd();

public slots:
    void onGetFreeAccount();

private slots:
    void onUpdateSystemFont(const QFont &);
    void onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &);

private:
    QString m_activityUrl;
    bool m_freeAccount;

    ThemedLable *m_pActivity = nullptr;
    UosFreeAccountActivity m_hasActivity;
    QSharedPointer<QFutureWatcher<QNetworkReply::NetworkError>> m_watcher;

};

#endif // GETFREEACCOUNTDIALOG_H
