#ifndef WIZARDWRAPPER_H
#define WIZARDWRAPPER_H
#include "uosai_global.h"

#include <DIconButton>
#include <DWidget>
#include <DMainWindow>
#include <DGuiApplicationHelper>
#include <DBlurEffectWidget>
#include <DLineEdit>
#include <DMenu>
#include <DPlatformWindowHandle>

#include <QVariant>
#include <QPropertyAnimation>
#include <QToolTip>
#include <QMouseEvent>
#include <QWidgetAction>
#include <QLabel>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

namespace  uos_ai {

class CustomDMenu : public DTK_WIDGET_NAMESPACE::DMenu
{
    Q_OBJECT
public:
    explicit CustomDMenu(QWidget *parent = nullptr) : DTK_WIDGET_NAMESPACE::DMenu(parent)
    {
        setMouseTracking(true);
    }

protected:
    void leaveEvent(QEvent *event) override
    {
        QToolTip::hideText();
        DTK_WIDGET_NAMESPACE::DMenu::leaveEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        QAction *action = actionAt(event->pos());
        if (action && !action->isEnabled() && !action->toolTip().isEmpty()) {
            QToolTip::showText(event->globalPos(), action->toolTip(), this);
        } else {
            QToolTip::hideText();
        }
        DTK_WIDGET_NAMESPACE::DMenu::mouseMoveEvent(event);
    }
};

class CustomButton;
class WizardDPushButton;
class InputPlaceholderWidget;
class WizardWrapper : public DTK_WIDGET_NAMESPACE::DBlurEffectWidget
{
    Q_OBJECT
public:
    static WizardWrapper &instance();
    void initAnimation();
    void showWizardWrapperWithAnimation();
    void isEnabledAction(bool isEnabled);
    void updateCustomFunctions();
    bool isMouseInside() { return m_mouseInside; }
    bool isWidgetVisible() { return m_isVisible; }
    void setWidgetVisible(bool visible) {  m_isVisible = visible; }
    void switchToInitMode();
    void switchToExpandedMode();
    void collapseWithAnimation();
    void setKnowledgeActionEnabled(bool enabled);
    void showToast(const QString &message);

protected:
    bool eventFilter(QObject *watched, QEvent *event)override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
#ifdef COMPILE_ON_QT6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

signals:
    void signalFunctionTriggered(int wizardtype, QPoint cursorPos, bool isCustom);
    void signalRequestServer();
    void signalCloseBtnClicked();
    void signalIconBtnClicked();
    void signalEscEvent();
    void signalDisableInApp();
    void signalDisableInProcess();
    void signalShowInputWindow(const QPoint &pos, const QRect &screenRect, int wizardWidth, int wizardHeight);

public slots:
    void showScribeWordsAtCursorPosition(QRect screenRect, QPoint& point, bool isMouseRelease, bool isShortcut);
    void onMoreMenuTriggered(const QAction *action);
    void onDisableMenuTriggered(const QAction *action);
    void onButtonClicked();
    void showMenu();
    void onCloseWidget();
    void onIconBtnClicked();
    void onInputAreaClicked();

private:
    WizardWrapper(QWidget *parent = nullptr);
    void onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &);
    void initUI();
    void initConnect();
    void adjustShowPos(const QRect &screenRect, QPoint &pos);
    bool isFirstClose();
    void expandWrapper();
    void createDynamicButtons();
    void updateButtonsFromCustomFunctions();
    uint calculateCustomFunctionListHash() const;
    void checkAndUpdateLayout();
    int calculateWidth();
    void clearLayout();
    QAction* createCustomMenuAction(const QString& text, const QString& iconName);
    void updateHoverStyle(QWidget* widget, bool hovered);
    Q_DISABLE_COPY(WizardWrapper)

    DTK_WIDGET_NAMESPACE::DIconButton *m_iconBtn = nullptr;
    InputPlaceholderWidget *m_inputArea = nullptr;
    QList<WizardDPushButton *> m_functionButtons;
    WizardDPushButton *m_moreBtn = nullptr;
    WizardDPushButton *m_closeBtn = nullptr;

    CustomDMenu *m_moreMenu = nullptr;
    CustomDMenu *m_disableMenu = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_lineSep = nullptr;
    QLabel *m_twoLineSep = nullptr;
    QAction *m_disableInProcessAction = nullptr;
    QAction *m_disableInAppAction = nullptr;
    QAction *m_disableGlobalAction = nullptr;
    QAction *m_customSettings = nullptr;
    QWidgetAction *m_settingAction = nullptr;
    QLabel *m_settingLabel = nullptr;

    QList<QAction *> m_customActions;

    QPoint m_cursorPos;
    QRect m_screenRect;
    bool m_isExtend = false;
    QPropertyAnimation *m_animation = nullptr;
    QPropertyAnimation *m_collapseAnimation = nullptr;
    QPropertyAnimation *m_scaleAnim = nullptr;
    bool m_mouseInside = false;
    bool m_isVisible = false;

    bool m_dragging = false;
    QPoint m_dragStartPos;

    uint m_customFunctionListHash = 0;
    
    QList<QWidget*> m_customMenuWidgets;
    QList<QString> m_customMenuWidgetIconNames;

    DPlatformWindowHandle::EffectScene m_effectScene;
};
}
#endif // WIZARDWRAPPER_H
