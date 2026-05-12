#ifndef COMMONFAILDIALOG_H
#define COMMONFAILDIALOG_H

#include <DAbstractDialog>
#include <DPlainTextEdit>
#include <DWidget>


namespace uos_ai {
class CommonFailDialog : public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT

public:
    explicit CommonFailDialog(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);

    void setFailMsg(const QString &msg);

private:
    void initUI();
    QString getErrorMsg(const int code);

private:

    DTK_WIDGET_NAMESPACE::DPlainTextEdit *m_pPlainTextEdit = nullptr;
};

}
#endif // COMMONFAILDIALOG_H
