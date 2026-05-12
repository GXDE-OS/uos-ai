#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include <DAboutDialog>

#include <QLabel>
#include <QRect>
#include <QWidget>

namespace uos_ai {

class AboutWindow : public DTK_WIDGET_NAMESPACE::DAboutDialog
{
    Q_OBJECT

public:
    explicit AboutWindow(QWidget *parent = nullptr);
    ~AboutWindow() override;

    void showDialog();

private slots:
    void onFontChanged();

private:
    void setupDialogContent();
    void adjustRecordNumberLabel();

    QLabel *m_recordTitleLabel = nullptr;
    QLabel *m_recordNumLabel = nullptr;
    static const QString RECORD_NUMBER_TEXT;
};

}
#endif // ABOUTWINDOW_H
