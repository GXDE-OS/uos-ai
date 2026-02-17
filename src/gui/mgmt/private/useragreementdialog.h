#ifndef USERAGREEMENTDIALOG_H
#define USERAGREEMENTDIALOG_H

#include <DAbstractDialog>
#include <DWidget>
#include <DArrowRectangle>
#include <DIconButton>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class WrapCheckBox;

class UserAgreementDialog : public DAbstractDialog
{
    Q_OBJECT

public:
    explicit UserAgreementDialog(DWidget *parent = nullptr);

private:
    void initUI();

    DArrowRectangle *showArrowRectangle(DArrowRectangle::ArrowDirection);

    QString getAgreementText();
private:
    DPushButton *m_pPushButton{nullptr};
};

#endif // USERAGREEMENTDIALOG_H
