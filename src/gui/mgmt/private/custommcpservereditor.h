#ifndef CUSTOMMCPSERVEREDITOR_H
#define CUSTOMMCPSERVEREDITOR_H

#include <QObject>
#include <QString>
#include <QJsonObject>

#include <DAbstractDialog>
#include <DWidget>
#include <DLabel>
#include <DLineEdit>
#include <DTextEdit>
#include <DPushButton>
#include <DSuggestButton>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>

DWIDGET_USE_NAMESPACE

namespace uos_ai {

class LineNumberTextEdit;
class CustomMcpServerEditor : public DAbstractDialog
{
    Q_OBJECT

    enum EditorMode {
        Add,
        Edit
    };

public:
    explicit CustomMcpServerEditor(DWidget *parent = nullptr);

    bool showAddMode();
    bool showEditMode(const QJsonObject &serverConfig, const QString &description);

    QJsonObject getServerConfig() const;
    void setTitle(const QString &title); 

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onOkButtonClicked();
    void onCancelButtonClicked();
    void updateOkButtonStatus();

private:
    void initUI();
    void initConnect();
    bool validateJsonConfig() const;
    QJsonObject assembleDescritions(const QJsonObject &config) const;

    QString defualtJsonConfig() const;
    void showErrorMessage(const QString &errMsg);

private:
    DLabel *m_titleLabel = nullptr;
    LineNumberTextEdit *m_jsonConfigEdit = nullptr;
    DTextEdit *m_descriptionEdit = nullptr;
    DPushButton *m_cancelButton = nullptr;
    DSuggestButton *m_okButton = nullptr;
    DLabel *m_errorLabel = nullptr;

    EditorMode m_currentMode = Add;
};

} // namespace uos_ai

#endif // CUSTOMMCPSERVEREDITOR_H
