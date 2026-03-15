#ifndef UPLOADFILESALERTDIALOG_H
#define UPLOADFILESALERTDIALOG_H

#include <DAbstractDialog>
#include <QEvent>

class QAbstractButton;
class QVBoxLayout;
namespace uos_ai {

class UploadFilesAlertDialog : public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT
public:
    explicit UploadFilesAlertDialog(QWidget *parent = nullptr);

    void showOnlineModelAlert(const QString &modelName);
    void showLocalModelAlert();

protected:
    void changeEvent(QEvent *event) override;

private:
    void initUI();
    void initButtons();

    void updateHeight();

    QAbstractButton *m_continueButton { nullptr };
    QAbstractButton *m_cancelButton { nullptr };
    QVBoxLayout *m_mainLayout { nullptr };
};

}

#endif   // UPLOADFILESALERTDIALOG_H
