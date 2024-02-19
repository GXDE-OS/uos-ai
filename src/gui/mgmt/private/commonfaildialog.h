#ifndef COMMONFAILDIALOG_H
#define COMMONFAILDIALOG_H

#include <DAbstractDialog>
#include <DPlainTextEdit>
#include <DWidget>

DWIDGET_USE_NAMESPACE

class CommonFailDialog : public DAbstractDialog
{
    Q_OBJECT

public:
    explicit CommonFailDialog(DWidget *parent = nullptr);

    void setFailMsg(const QString &msg);

private:
    void initUI();
    QString getErrorMsg(const int code);

private:

    DPlainTextEdit *m_pPlainTextEdit = nullptr;
};

#endif // COMMONFAILDIALOG_H
