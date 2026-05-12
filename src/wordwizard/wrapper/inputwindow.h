#ifndef INPUTWINDOW_H
#define INPUTWINDOW_H

#include "wizardwrapper.h"
#include <DBlurEffectWidget>
#include <DLineEdit>
#include <DIconButton>
#include <DWidget>
#include <QPropertyAnimation>
#include <QHBoxLayout>
#include <QTimer>
#include <QWidgetAction>
#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

namespace uos_ai {

class SendButton;
class WizardDPushButton;
class InputWindow : public DBlurEffectWidget
{
    Q_OBJECT

public:
    static InputWindow &instance();
    void showAtWizardWrapperPosition(const QPoint &pos, const QRect &screenRect, int wizardWidth, int wizardHeight);
    void expandWithAnimation();
    void isEnabledAction(bool isEnabled);
    bool isMouseInside() { return m_mouseInside; }

signals:
    void signalInputTextTriggered(const QString &inputText, const QString &clipboardText);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event)override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void signalCloseBtnClicked();
    void signalFunctionTriggered(int wizardtype, QPoint cursorPos, bool isCustom);
    void signalRequestServer();
    void signalDisableInApp();
    void signalDisableInProcess();

public slots:
    void onDisableMenuTriggered(const QAction *action);

private slots:
    void onSendButtonClicked();
    void onInputTextChanged();
    void onExpandAnimationFinished();
    void onCloseWidget();

private:
    InputWindow(QWidget *parent = nullptr);
    void initUI();
    void initConnect();
    void initAnimation();
    void adjustPosition(const QRect &screenRect);
    void updateTheme(const DGuiApplicationHelper::ColorType &themeType);
    bool isFirstClose();
    QAction* createCustomMenuAction(const QString& text, const QString& iconName);
    void updateHoverStyle(QWidget* widget, bool hovered);
    Q_DISABLE_COPY(InputWindow)

    DWidget *m_lineSep1 = nullptr;
    DWidget *m_lineSep2 = nullptr;
    DLineEdit *m_inputEdit = nullptr;
    QWidgetAction *m_sendBtnAction = nullptr;
    CustomDMenu *m_disableMenu = nullptr;
    QAction *m_disableInProcessAction = nullptr;
    QAction *m_disableInAppAction = nullptr;
    QAction *m_disableGlobalAction = nullptr;
    QWidgetAction *m_settingAction = nullptr;
    QLabel *m_settingLabel = nullptr;
    SendButton *m_sendBtn = nullptr;
    WizardDPushButton *m_closeBtn = nullptr;
    QPropertyAnimation *m_expandAnimation = nullptr;
    QPropertyAnimation *m_scaleAnim = nullptr;
    QPoint m_showPos;
    QRect m_screenRect;
    bool m_isVisible = false;
    int m_wizardWidth = 0;
    int m_wizardHeight = 0;
    bool m_dragging = false;
    bool m_mouseInside = false;
    QPoint m_dragStartPos;

    QList<QWidget*> m_customMenuWidgets;
    QList<QString> m_customMenuWidgetIconNames;
};

}

#endif // INPUTWINDOW_H 
