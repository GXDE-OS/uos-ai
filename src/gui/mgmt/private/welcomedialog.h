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
#include <DApplicationHelper>

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
    explicit WelcomeDialog(DWidget *parent = nullptr, bool onlyUseAgreement = false);

    bool isFreeAccount();
    Qt::CheckState getUserExpState();
    void resetDialog();
    void setOnlyUseAgreement(bool onlyUseAgreement) { m_onlyUseAgreement = onlyUseAgreement; }

private:
    void initUI();
    void initConnect();

    DArrowRectangle *showArrowRectangle(DArrowRectangle::ArrowDirection);

    void resetLinkColor();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void signalAppendModel(const LLMServerProxy &);

public slots:
    void onGetFreeAccount();

private slots:
    void onUpdateSystemFont(const QFont &);
    void onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &);

private:
    WrapCheckBox *m_pAgrCheckbox{nullptr};
    WrapCheckBox *m_pExpCheckbox{nullptr};
    DIconButton *m_pExpIcon{nullptr};

    DSuggestButton *m_pFreeAccount{nullptr};
    DSuggestButton *m_pStartUsing{nullptr};
    DPushButton *m_pAddModel{nullptr};

    ThemedLable *m_pActivity{nullptr};

    DWidget *m_pFreeWidget{nullptr};

    bool m_freeAccount;
    QString m_activityUrl;
    UosFreeAccountActivity m_hasActivity;
    QSharedPointer<QFutureWatcher<QNetworkReply::NetworkError>> m_watcher;

    bool m_onlyUseAgreement = false;
};

#endif // WELCOMEDIALOG_H
