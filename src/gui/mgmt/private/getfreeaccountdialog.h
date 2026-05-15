#ifndef GETFREEACCOUNTDIALOG_H
#define GETFREEACCOUNTDIALOG_H

#include "uosfreeaccounts.h"

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

class WrapCheckBox;

namespace uos_ai {

class ThemedLable;

class GetFreeAccountDialog : public DTK_WIDGET_NAMESPACE::DDialog
{
    Q_OBJECT
public:
    explicit GetFreeAccountDialog(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);

    void resetDialog();

private:
    void initUI();
    void initConnect();
    void resetLinkColor();

signals:
    void freeModelAppend();
    void signalActivityEnd();

public slots:
    void onGetFreeAccount();

private slots:
    void onUpdateSystemFont(const QFont &);
    void onUpdateSystemTheme(const DTK_GUI_NAMESPACE::DGuiApplicationHelper::ColorType &);

private:
    QString m_activityUrl;

    ThemedLable *m_pActivity = nullptr;
    UosFreeAccountActivity m_hasActivity;
    QSharedPointer<QFutureWatcher<QNetworkReply::NetworkError>> m_watcher;

};

}
#endif // GETFREEACCOUNTDIALOG_H
