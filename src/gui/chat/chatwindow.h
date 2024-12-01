#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <DBlurEffectWidget>
#include <DLabel>
#include <DWindowManagerHelper>
#include <DRegionMonitor>
#include <DFloatingMessage>
#include <DMainWindow>
#include <DTitlebar>
#include <DToolButton>
#include <DImageButton>
#include <DWindowOptionButton>
#include <QDBusInterface>

#include <QDir>
#include <QTime>
#include <QAnimationGroup>

#include "private/echatbutton.h"

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
DCORE_USE_NAMESPACE

class ESingleWebView;
class QStackedWidget;
class QPropertyAnimation;
class QSequentialAnimationGroup;

#ifdef COMPILE_ON_V23
class DBusDisplay;
class DBusDock;
#else
class __Display;
class DDeDockObject;
#endif

class __Appearance;

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

class ChatWindow : public DMainWindow
{
    Q_OBJECT
    Q_PROPERTY(int width READ width WRITE setW)
    Q_PROPERTY(int x READ x WRITE setX)
public:
    enum DisplayMode {
        SIDEBAR_MODE = 0,
        WINDOW_MODE = 1
    };

    explicit ChatWindow(QWidget *parent = nullptr);
    void initTitlebar();
    void showWindowMode();
    void showSidebarMode();

    //update location and show/hide
    void showWindow(ChatIndex index);

    //showToast Show temporary toast message
    void showToast(const QString &message);

    //call when JS X button clicked
    void closeWindow();

    void showAboutWindow();
    void setMenuDisabled(bool disabled);
    void setVoiceConversationDisabled(bool disabled);
    void setWindowTitleVisible(bool visible);
    void setWindowIconVisible(bool visible);
    void setHasChatHistory(bool hasChatHistory);

protected:
    //update location and show
    void onlyShowWindow();

    //rewrite mouseMoveEvent to block the mouse move event.
    void mouseMoveEvent(QMouseEvent *event) override;

    //send chat window going be hiden event
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void onlyShowWidget();
    void updateSystemTheme();
    void updateSidebarGeometry(int currentWidth = SidebarDefaultWidth);
    void setW(int w);
    void setX(int x);
    void activeShowWindow();

public Q_SLOTS:
    void onSystemThemeChanged();
    void onScreenOrDockChanged();
    void onAudioRecShortcutPressed();
//    void onAnimationStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);
    void onModalStateChanged(bool modal);
    void onMenuTriggered(QAction *action);
    void onChatBtnClicked();
    void onVoiceConversationStatusChanged(int status);
    void onGenPersonalFAQ();

private slots:
    void appearancePropertiesChanged(QString, QVariantMap, QStringList);

signals:
    void sigToLaunchChat(int);
    void sigToLaunchAbout();

private:
    QWidget *m_background = nullptr;
    ESingleWebView *m_webView = nullptr;
    QAction *m_gloablShortcut = nullptr;
    QStackedWidget *m_stackedWidget;
private:
    static const int SidebarDefaultWidth = 400;
    static const int DefaultMargin = 10;  //10->0
    static const int AnimationTime = 300;

    QRect m_displayRect;
    QRect m_geometry;


#ifdef COMPILE_ON_V23
    DBusDisplay *m_display = nullptr;
    DBusDock *m_dock = nullptr;
#else
    __Display *m_display = nullptr;
    DDeDockObject *m_dock = nullptr;
#endif
    __Appearance *m_appearance = nullptr;
    QAction *m_windowModeAction = nullptr;
    QAction *m_sidebarModeAction = nullptr;
    QAction *m_settingsAction = nullptr;

    EChatButton *m_chatBtn = nullptr;

    Qt::WindowFlags m_wWindowFlags;
    Qt::WindowFlags m_sWindowFlags;

    DisplayMode m_displayMode = SIDEBAR_MODE;
    bool m_isDigitalMode = false;
    QSize m_windowSize = QSize(680, 900); // 窗口模式下窗口尺寸
    // 1:silence, 2:listen, 3:think, 4:input, 5:network error, 6:input device error,
    // 7:stop recording, 8:account error, 9:recording error, 10:output device error
    int m_voiceConversationStatus = 0;
    bool m_isMaximized = false;
    bool m_hasChatHistory = false;
    bool m_voiceConversationDisabled = false;
    bool m_hasBlurWindow = false;

    QDBusInterface *m_appearanceInter;
    double m_alpha = 0.6;
};

#endif // CHATWINDOW_H
