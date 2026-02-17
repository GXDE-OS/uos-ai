#ifndef ECHECKAGREEMENTDIALOG_H
#define ECHECKAGREEMENTDIALOG_H

#include "serverdefs.h"

#include <DAbstractDialog>
#include <DWidget>
#include <DLabel>
#include <DComboBox>
#include <DLineEdit>
#include <DPushButton>
#include <DTitlebar>
#include <DFontSizeManager>
#include <DPasswordEdit>
#include <DSuggestButton>
#include <DSpinner>
#include <DCheckBox>

#include <QGridLayout>
#include <QHBoxLayout>

DWIDGET_USE_NAMESPACE

namespace uos_ai {
class ECheckAgreementDialog : public DAbstractDialog
{
    Q_OBJECT
public:
    explicit ECheckAgreementDialog(DWidget *parent = nullptr);

signals:

public slots:
    void onCheckBoxStateChanged(int);
    void onCancelButtonClicked();
    void onConfirmButtonClicked();
    void onUpdateSystemFont(const QFont &);
private:
    void initUI();
    void initConnect();

private:
    DCheckBox *m_pCheckBox = nullptr;
    DPushButton *m_pCancelButton = nullptr;
    DSuggestButton *m_pConfiemButton = nullptr;
};
}

#endif // ECHECKAGREEMENTDIALOG_H
