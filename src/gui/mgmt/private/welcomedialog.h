#ifndef WELCOMEDIALOG_H
#define WELCOMEDIALOG_H

#include <DAbstractDialog>
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

class WelcomeDialog : public DAbstractDialog
{
    Q_OBJECT
public:
    static WelcomeDialog *instance(bool onlyUseAgreement = false);

    bool isFreeAccount();
    void resetDialog();
    static bool isAgreed();
    inline void setOnlyUseAgreement(bool onlyUseAgreement) { m_onlyUseAgreement = onlyUseAgreement; }
    inline bool isOnlyUseAgreement() { return m_onlyUseAgreement; }

private:
    void initUI();
    void initConnect();

    DArrowRectangle *showArrowRectangle(DArrowRectangle::ArrowDirection);

    void resetLinkColor();
    void updateAgree();

signals:
    void signalAppendModel(const LLMServerProxy &);
    //当用户选择添加模型时，需要先拉起chatwindow，在chatwindow绘制完成后在拉起设置界面。
    //否则会导致设置界面被覆盖在chatwindow下面且无法激活。
    void signalShowMgmtWindowAfterChatInitFinished();

public slots:
    void onGetFreeAccount();

private slots:
    void onUpdateSystemFont(const QFont &);
    void onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &);

protected:
    void showEvent(QShowEvent *event) override;

private:
    WelcomeDialog(DWidget *parent = nullptr, bool onlyUseAgreement = false);
    WrapCheckBox *m_pAgrCheckbox{nullptr};

    DSuggestButton *m_pFreeAccount{nullptr};
    DSuggestButton *m_pStartUsing{nullptr};
    DPushButton *m_pAddModel{nullptr};

    ThemedLable *m_pActivity{nullptr};
    ThemedLable *m_pIntroduce{nullptr};
    QSpacerItem *m_pVerticalSpacer{nullptr};

    DWidget *m_pFreeWidget{nullptr};

    bool m_freeAccount;
    QString m_activityUrl;
    UosFreeAccountActivity m_hasActivity;
    QSharedPointer<QFutureWatcher<QNetworkReply::NetworkError>> m_watcher;

    bool m_onlyUseAgreement = false;
};

#endif // WELCOMEDIALOG_H
