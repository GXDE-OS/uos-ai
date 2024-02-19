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
    void initConnect();

    DArrowRectangle *showArrowRectangle(DArrowRectangle::ArrowDirection);

    QString getAgreementText();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    WrapCheckBox *m_pExpCheckbox{nullptr};

    DPushButton *m_pPushButton{nullptr};
    DIconButton *m_pExpIcon{nullptr};
};

#endif // USERAGREEMENTDIALOG_H
