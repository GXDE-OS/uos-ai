#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include "private/echatbutton.h"
#include "private/emaskwidget.h"
#include "../knowledgebase/referencedialog.h"

#include <DBlurEffectWidget>
#include <DLabel>
#include <DWindowManagerHelper>
#include <DRegionMonitor>
#include <DFloatingMessage>
#include <DMainWindow>
#include <DTitlebar>
#include <DToolButton>
#include <DWindowOptionButton>
#include <QDBusInterface>

#include <QDir>
#include <QTime>
#include <QAnimationGroup>

#include <mutex>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
DCORE_USE_NAMESPACE

class ESingleWebView;
class QStackedWidget;
class QPropertyAnimation;
class QSequentialAnimationGroup;
class DDeDockObject;
class EMaskWidget;

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

enum ChatInitAsyncTask {
    ParserDocument = 1,
    OverrideQuestion,
    AddKnowledgeBase,
    AddAskQuestion,
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
    void searchShortCut();

    //update location and show/hide
    void showWindow(ChatIndex index);

    //update location and show
    void onlyShowWindow();

    //showToast Show temporary toast message
    void showToast(const QString &message);

    //call when JS X button clicked
    void closeWindow();

    void showAboutWindow();
    void onAboutFontChanged();
    void setMenuDisabled(bool disabled);
    void setVoiceConversationDisabled(bool disabled);
    void setChatButtonVisible(bool visible);
    void setWindowTitleVisible(bool visible);
    void setWindowIconVisible(bool visible);
    void setHasChatHistory(bool hasChatHistory);

    bool isDigitalMode();
    void digital2ChatStatusChange();

    void previewReferenceDoc(const QString &docPath, const QStringList &docContents);

    void overrideQuestion(const QString &question, const QMap<QString, QString> &ext);
    void appendQuestion(const QString &question);
    void appendImage(const QString &imagePath);
    void appendAskQuestion(int assistantType);
    void addKnowledgeBase(const QStringList &knowledgeBasefile);

    void setTitleBarStatus(bool status);

    bool showWarningDialog(const QString assistantId, const QString conversationId, const QString msg, bool isDelete, bool isLlmDelete, bool isAllConvDelete);
    bool showRmMcpServerDlg(const QString &name);
    void showUpdateDialog(const QString &msg, const QString &appName);

    void startScreenshot();

    bool getThirdPartyMcpAgreement();
    static void setNeedShowLLMConfigWindow(bool isNeedShowLLMConfigWindow);

    void showKnowledgeBaseErrorDialog(int type, QString appName);

    bool showLostFileWarningDlg(const QString lostFileList);

    // mcp环境缺失弹窗
    bool showInstallUosAIAgentDlg(QString appName);

    // 获取titleBar()单个按钮宽度
    int getTitleBarBtnWidth();

    // 领取额外免费额度弹窗
    bool showGetFreeCreditsDlg();

    // 查询当前快捷键设置
    QString getCurrentShortcut();
    void showGetFreeCreditsResultDlg(bool isSuccess);
protected:
    //rewrite mouseMoveEvent to block the mouse move event.
    void mouseMoveEvent(QMouseEvent *event) override;

    //send chat window going be hiden event
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *pe)override;
#ifdef COMPILE_ON_QT6
    bool event(QEvent *event) override;
#endif

private:
    void onlyShowWidget();
    void updateSystemTheme();
    void updateSidebarGeometry(int currentWidth = SidebarDefaultWidth);
    void setW(int w);
    void setX(int x);
    void activeShowWindow();
    QString getLatestUpdateLogVersion();
    void checkForUpdateLogs();

public Q_SLOTS:
    void onSystemThemeChanged();
    void onScreenOrDockChanged();
    void onModalStateChanged(bool modal);
    void onMenuTriggered(QAction *action);
    void onChatBtnClicked();
    void onVoiceConversationStatusChanged(int status);
    void onGenPersonalFAQ();
    void onDocSummaryDragInView(const QStringList &docPaths);
    void onChatInitFinished();
    void onRedPointVisible(bool isVisible);

private slots:
    void appearancePropertiesChanged(QString, QVariantMap, QStringList);

signals:
    void sigToLaunchChat(int);
    void sigToLaunchAbout();
    void sigToAddKnowledgeBase(const QStringList &knowledgeBasefile);
    void sigThirdPartyMcpAgree();

private:
    ESingleWebView *m_webView = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    uos_ai::ReferenceDialog *m_knowBaseDialog = nullptr;
private:
    static const int SidebarDefaultWidth = 400;
    static const int DefaultMargin = 10;  //10->0
    static const int AnimationTime = 300;
    //是否在onChatInitFinished时打开设置窗口
    static bool s_isNeedShowLLMConfigWindow;

    QRect m_displayRect;
    QRect m_geometry;

    DDeDockObject *m_dock = nullptr;

    QAction *m_windowModeAction = nullptr;
    QAction *m_sidebarModeAction = nullptr;
    QAction *m_settingsAction = nullptr;
    QAction *m_updateLogAction = nullptr;

    EChatButton *m_chatBtn = nullptr;

    Qt::WindowFlags m_wWindowFlags;
    Qt::WindowFlags m_sWindowFlags;

    DisplayMode m_displayMode = WINDOW_MODE;
    bool m_isDigitalMode = false;
    QSize m_windowSize = QSize(1010, 750); // 窗口模式下窗口尺寸
    // 1:silence, 2:listen, 3:think, 4:input, 5:network error, 6:input device error,
    // 7:stop recording, 8:account error, 9:recording error, 10:output device error
    int m_voiceConversationStatus = 0;
    bool m_isMaximized = false;
    bool m_hasChatHistory = false;
    bool m_voiceConversationDisabled = false;
    bool m_hasBlurWindow = false;

    double m_alpha = 0.6;
    double m_minAlpha = 0.6;

    QColor m_backgroundColor;

    // chat init async task
    using AiTask = QPair<int, QVariantList>;
    QList<AiTask> m_pendingTasks;

    mutable std::once_flag m_shortcutUpdateDialogOnceFlag;

    bool m_needActiveWindow { true };
    QString m_currentShortcut;
};

#endif // CHATWINDOW_H
