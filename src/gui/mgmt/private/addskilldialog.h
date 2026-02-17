#ifndef ADDSKILLDIALOG_H
#define ADDSKILLDIALOG_H
#include "skillcommandtextedit.h"

#include <DAbstractDialog>
#include <DWidget>
#include <DLineEdit>
#include <DTextEdit>
#include <DPushButton>
#include <DSuggestButton>
#include <DSpinner>
#include <DLabel>
#include <DDialog>

#include <QSet>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QTimer>

namespace uos_ai {
class IconCommandLinkButton;
class AddSkillDialog : public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT

public:
    explicit AddSkillDialog(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);

    QString getSkillName() const;
    QString getSkillCommand() const;
    void resetDialog();
    void setSkillData(const QString &name, const QString &command);

private slots:
    void onSubmitButtonClicked();
    void onCancelButtonClicked();
    void onSkillNameChanged(const QString &text);
    void onSkillNameAlertChanged(bool alert);
    void onSkillCommandChanged();
    void onUpdateSubmitButtonStatus();
    void onCompactModeChanged();
    void onUpdateSystemFont(const QFont &);
    void onInsertSelectedContentTag();
    void onTagDeleted();
    void hideTooltip(bool isDisabled);
    void updateTooltipPosition();

private:
    void initUI();
    void initConnect();
    bool isSkillNameValid() const;
    bool isSkillCommandValid() const;
    void updateCharCountLabelPosition();
    void showInstructionDialog();
    void showNativeTooltip();
    bool isNameDuplicate() const;

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private:
    DTK_WIDGET_NAMESPACE::DLineEdit *m_pSkillNameLineEdit = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_pCancelButton = nullptr;
    DTK_WIDGET_NAMESPACE::DSuggestButton *m_pSubmitButton = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pCharCountLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pTooltipTextLabel = nullptr;
    SkillCommandTextEdit *m_pSkillCommandTextEdit = nullptr;
    ClickableLabel *m_pClickableLabel = nullptr;
    QWidget *m_pWidget = nullptr;
    IconCommandLinkButton *m_pInstructionButton = nullptr;
    QWidget *m_pTooltipWidget = nullptr;

    QString m_skillName;
    QString m_skillCommand;
    QString m_originalSkillName;
};
}

#endif // ADDSKILLDIALOG_H
