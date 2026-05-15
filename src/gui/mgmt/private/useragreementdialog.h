#ifndef USERAGREEMENTDIALOG_H
#define USERAGREEMENTDIALOG_H

#include <DAbstractDialog>
#include <DWidget>
#include <DArrowRectangle>
#include <DIconButton>
#include <DLabel>

class WrapCheckBox;
namespace uos_ai {
class UserAgreementDialog : public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT

public:
    explicit UserAgreementDialog(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);

private:
    void initUI();

    DTK_WIDGET_NAMESPACE::DArrowRectangle *showArrowRectangle(DTK_WIDGET_NAMESPACE::DArrowRectangle::ArrowDirection);

    QString getAgreementText();
private:
    DTK_WIDGET_NAMESPACE::DPushButton *m_pPushButton{nullptr};
};
}
#endif // USERAGREEMENTDIALOG_H
