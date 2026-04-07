#ifndef SKILLSERVERLISTITEM_H
#define SKILLSERVERLISTITEM_H

#include "uosai_global.h"
#include "skillsmanager.h"

#include <DWidget>
#include <DLabel>
#include <DSwitchButton>
#include <DIconButton>

DWIDGET_USE_NAMESPACE

namespace uos_ai {
    
class SkillServerListItemTooltip;
class SkillServerListItem: public DWidget
{
    Q_OBJECT

public:
    explicit SkillServerListItem(DWidget *parent = nullptr);
    ~SkillServerListItem();

    void setText(const QString &name, const QString &description);
    void setSwitchChecked(bool checked);
    void setBuiltIn(bool builtIn);
    void setServerId(const QString &serverId);
    void setDeletable(bool deletable);
    QString serverId();

signals:
    void signalDelete(const QString &name, bool isDeletable);
    void signalAgreementAccepted();

private slots:
    void onDelete();
    void onSwitchChanged(bool checked);

protected:
#ifdef COMPILE_ON_QT6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void initUI();
    void initConnect();
    void updateButtonVisibility();
    void showCustomToolTip();
    void hideCustomToolTip(int delay = 0);
    bool getThirdPartyMcpAgreement();

private:
    DLabel *m_pNameLabel = nullptr;
    DLabel *m_pDescLabel = nullptr;
    DLabel *m_pBuiltInLabel = nullptr;
    DIconButton *m_pBtnDelete = nullptr;
    DSwitchButton *m_pBtnSwitch = nullptr;
    QWidget *m_pEditPlaceholder = nullptr;
    QWidget *m_pDeletePlaceholder = nullptr;
    SkillServerListItemTooltip *m_pCustomToolTip = nullptr;
    QTimer *m_pTooltipTimer = nullptr;

    QString m_serverId;
    QString m_currentName;
    QString m_currentDescription;
    bool m_isBuiltIn = false;
    bool m_isHovered = false;
    bool m_isDeletable = true;
protected:
    QScopedPointer<SkillsManager> m_skills;

};

}

#endif // SKILLSERVERLISTITEM_H
