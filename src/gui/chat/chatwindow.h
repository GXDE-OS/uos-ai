#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <DBlurEffectWidget>
#include <DLabel>
#include <DWindowManagerHelper>
#include <DRegionMonitor>
#include <DFloatingMessage>
#include <QDir>
#include <QTime>
#include <QAnimationGroup>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

class ESingleWebView;
class QStackedWidget;
class QPropertyAnimation;
class QSequentialAnimationGroup;

class __Appearance;
class __Dock;
class __Display;

class DDeDockObject;

enum DockPosition {
    Top = 0,
    Right = 1,
    Bottom = 2,
    Left = 3
};

enum DockModel {
    Fashion = 0,
    Efficient = 1
};

enum ChatIndex {
    Text,
    Talk
};

class ChatWindow : public DBlurEffectWidget
{
    Q_OBJECT
    Q_PROPERTY(int width READ width WRITE setW)
    Q_PROPERTY(int x READ x WRITE setX)
public:
    explicit ChatWindow(QWidget *parent = nullptr);

    //update location and show/hide
    void showWindow(ChatIndex index);

    //showToast Show temporary toast message
    void showToast(const QString &message);

    //call when JS X button clicked
    void closeWindow();

    void showAboutWindow();
protected:
    //update location and show
    void onlyShowWindow();

    //rewrite mouseMoveEvent to block the mouse move event.
    void mouseMoveEvent(QMouseEvent *event) override;

    //send chat window going be hiden event
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    void onlyShowWidget();
    void onlyHideWidget();
    void updateSystemTheme();
    void updateGeometry(int currentWidth = DefaultWidth);
    void setW(int w);
    void setX(int x);
    void activeShowWindow();

public Q_SLOTS:
    void onSystemThemeChanged();
    void onScreenOrDockChanged();
    void onAudioRecShortcutPressed();
    void onAnimationStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);
    void onModalStateChanged(bool modal);

private:
    QWidget *m_background = nullptr;
    QLabel *m_fakeView = nullptr;
    ESingleWebView *m_webView = nullptr;
    QAction *m_gloablShortcut = nullptr;
    QStackedWidget *m_stackedWidget;
private:
    static const int DefaultWidth = 400;
    static const int DefaultMargin = 10;  //10->0
    static const int AnimationTime = 300;

    QPropertyAnimation *m_xAnimation = nullptr;
    QPropertyAnimation *m_wAnimation = nullptr;
    QSequentialAnimationGroup *m_animationGroup = nullptr;

    QRect m_displayRect;
    QRect m_geometry;

    __Appearance *m_appearance = nullptr;
    __Display *m_display = nullptr;

    DDeDockObject *m_dock = nullptr;
};

#endif // CHATWINDOW_H
