#include "chatwindow.h"
#include "private/esinglewebview.h"
#include "private/eaiexecutor.h"
#include "private/echatwndmanager.h"
#include "private/uploadfilesalertdialog.h"
#include "../common/echeckagreementdialog.h"
#include "dbwrapper.h"
#include "private/echatbutton.h"
#include "serverwrapper.h"
#include "private/welcomedialog.h"
#include "private/easyncworker.h"
#include "oscallcontext.h"
#include "ddedockobject.h"
#include "tables/configtable.h"
#include "util.h"
#include "global_define.h"
#ifdef COMPILE_ON_V25
#include "dlayershellwindow.h"
#include <ddeshellwayland.h>
#endif
#include "esystemcontext.h"
#include "gui/upgrade/shortcutupdatedialog.h"
#include "gui/upgrade/updatelogdialog.h"
#include "dbus/shortcutmanager.h"
#include <wordwizard.h>
#include <report/chatwindowpoint.h>
#include <report/eventlogutil.h>
#include <agentfactory.h>
#include <mcpserver.h>

#include <QBoxLayout>
#include <QDBusInterface>
#include <QDBusReply>
#include <QPalette>
#include <QTimer>
#include <QScreen>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QDir>
#include <QAction>
#include <QStackedLayout>
#include <QApplication>
#include <QMenu>
#include <QResizeEvent>
#include <QActionGroup>
#include <QList>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QFontMetrics>

#include <DLabel>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DHiDPIHelper>
#include <DAboutDialog>
#include <DWindowOptionButton>
#include <DWidget>
#include <DWidgetUtil>
#include <DWindowManagerHelper>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

static constexpr char appIcon[] = "uos-ai-assistant";

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

bool ChatWindow::s_isNeedShowLLMConfigWindow = false;

ChatWindow::ChatWindow(QWidget *parent)
    : DMainWindow(parent)
    , m_webView(new ESingleWebView(this))
{
    m_wWindowFlags = windowFlags();
    // BUG-322453：为解决遮挡问题，将Qt::Tool换为Qt::Dialog，保持与mgmt相同
    m_sWindowFlags = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint;
#ifdef ENABLE_AI_BAR
    if (ESystemContext::isTreeland()) {
        m_wWindowFlags = (windowFlags() | Qt::Dialog) & ~Qt::WindowMinimizeButtonHint;
    } else {
        m_wWindowFlags = windowFlags() | Qt::Dialog;
    }

    m_sWindowFlags = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint;
#endif

    if (windowHandle()) {
        //windowHandle()->setProperty("_d_dwayland_window-type", "standAlone");
#ifdef ENABLE_AI_BAR
        if (ESystemContext::isTreeland()) {
            DDEShellWayland::get(windowHandle())->setSkipDockPreview(true);
        }
#endif
    }

    // 确保窗口的深色模式和浅色模式跟随系统设置改变
    DGuiApplicationHelper::ColorType type = DGuiApplicationHelper::UnknownType;
    DGuiApplicationHelper::instance()->setPaletteType(type);

    m_dock = new DDeDockObject(this);
    m_hasBlurWindow = DWindowManagerHelper::instance()->hasBlurWindow();

    if (DbWrapper::localDbWrapper().getDisplayMode() == 1) {
        m_displayMode = WINDOW_MODE;
        m_webView->setWindowMode(true);
    }else {
        m_displayMode = SIDEBAR_MODE;
        m_webView->setWindowMode(false);
    }
    QStringList sizeList = DbWrapper::localDbWrapper().getWindowSize().split("#");
    if (sizeList.size() == 2) {
        int width = sizeList[0].toInt();
        int height = sizeList[1].toInt();
        if (width > 0 && height > 0) {
            m_windowSize = QSize(width, height);
        }
    }

    initTitlebar();

    QFontInfo fontInfo = this->fontInfo();
    m_webView->updateFont(fontInfo.family(), fontInfo.pixelSize());
    m_webView->setAccessibleName("webview");
    m_webView->page()->setBackgroundColor(Qt::transparent);
    m_webView->setUrl(QUrl("qrc:/assets/web/front/dist/index.html"));

    m_stackedWidget = new QStackedWidget;
    m_stackedWidget->addWidget(m_webView);
    m_stackedWidget->setAttribute(Qt::WA_TranslucentBackground);
    setCentralWidget(m_stackedWidget);

    auto conn = QDBusConnection::sessionBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(osCallDbusAppearanceService, osCallDbusAppearancePath, QString("org.freedesktop.DBus.Properties"), QString("Get"));
    QVariantList args;
    args << QVariant::fromValue(QString(osCallDbusAppearanceInterface))
         << QVariant::fromValue(QString("Opacity"));
    msg.setArguments(args);
    QDBusReply<QVariant> reply = conn.call(msg, QDBus::CallMode::Block, 100);
    if (reply.isValid()) {
        m_alpha = reply.value().toDouble();
    }

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ChatWindow::onSystemThemeChanged);

    connect(m_dock, &DDeDockObject::FrontendWindowRectChanged, this, &ChatWindow::onScreenOrDockChanged);

    updateSystemTheme();
    EAiExec()->setChatWindow(this);
    EAiExec()->getHistoryFromChatInfo();
    EAiExec()->createNewConversation();

    installEventFilter(this);

    connect(EWndManager(), &EChatWndManager::modalStateChanged, this, &ChatWindow::onModalStateChanged);

    connect(m_webView, &ESingleWebView::voiceConversationStatusChanged, this, &ChatWindow::onVoiceConversationStatusChanged);
    connect(m_webView, &ESingleWebView::docSummaryDragInView, this, &ChatWindow::onDocSummaryDragInView);
    connect(m_webView, &ESingleWebView::sigChatInitFinished, this, &ChatWindow::onChatInitFinished);

    connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasBlurWindowChanged, this, [=](){
        // qDebug() << "hasBlurWindowChanged: " << DWindowManagerHelper::instance()->hasBlurWindow();
        m_hasBlurWindow = DWindowManagerHelper::instance()->hasBlurWindow();
        updateSystemTheme();
    });

    QDBusConnection session = QDBusConnection::sessionBus();
    session.connect(osCallDbusAppearanceService, osCallDbusAppearancePath, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                    this, SLOT(appearancePropertiesChanged(QString, QVariantMap, QStringList)));

    searchShortCut();
}

void ChatWindow::appearancePropertiesChanged(QString interface, QVariantMap changedProperties, QStringList)
{
    if (interface != osCallDbusAppearanceInterface)
        return;

    for (auto iter = changedProperties.begin(); iter != changedProperties.end(); iter++) {
        if (iter.key() == "Opacity") {
            m_alpha = qdbus_cast<double>(iter.value());
            qCDebug(logAIGUI) << "Window opacity changed to:" << m_alpha;
            updateSystemTheme();
        }
    }
}

void ChatWindow::initTitlebar()
{
    QMenu *titleMenu = new QMenu(this);
    QMenu *m_displayModeMenu = new QMenu(tr("Mode"));
    QActionGroup *m_displayModeActionGroup = new QActionGroup(m_displayModeMenu);
    m_windowModeAction = new QAction(tr("Window Mode"));
    m_windowModeAction->setActionGroup(m_displayModeActionGroup);
    m_displayModeMenu->addAction(m_windowModeAction);
    m_windowModeAction->setCheckable(true);

    m_sidebarModeAction = new QAction(tr("Sidebar Mode"));
    m_sidebarModeAction->setActionGroup(m_displayModeActionGroup);
    m_displayModeMenu->addAction(m_sidebarModeAction);
    m_sidebarModeAction->setCheckable(true);

    m_settingsAction = new QAction(tr("Settings"));
    m_updateLogAction = new QAction(tr("Update Log"));

    titleMenu->addMenu(m_displayModeMenu);
    titleMenu->addAction(m_settingsAction);
    titleMenu->addSeparator();

    if (m_displayMode == WINDOW_MODE) {
        m_windowModeAction->setChecked(true);
    } else {
        m_sidebarModeAction->setChecked(true);
    }

    titlebar()->setIcon(QIcon::fromTheme(kApplicationIconName)); //relate to setWindowIconVisible
    titlebar()->setMenu(titleMenu);
    titlebar()->setSwitchThemeMenuVisible(false);
    titlebar()->setQuitMenuVisible(false);
    titlebar()->setBackgroundTransparent(false);

    auto actList = titlebar()->menu()->actions();
    QAction *aboutAction = actList.at(actList.length() - 2);
    titlebar()->menu()->insertAction(aboutAction, m_updateLogAction);

    m_chatBtn = new EChatButton(this);
    m_chatBtn->setToolTip(tr("Voice conversation"));
    //m_chatBtn->setIcon(QIcon(QString(":/icons/deepin/builtin/dark/icons/chat.svg")));
    m_chatBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_chatBtn->setIconSize(QSize(titlebar()->height(), titlebar()->height()));
    titlebar()->addWidget(m_chatBtn, Qt::AlignRight);

    connect(m_chatBtn, &DIconButton::clicked, this, &ChatWindow::onChatBtnClicked);
    connect(titleMenu, &QMenu::triggered, this, &ChatWindow::onMenuTriggered);
#ifdef DTKWIDGET_CLASS_DSizeMode
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, [=](DGuiApplicationHelper::SizeMode sizeMode){
        m_chatBtn->setIconSize(QSize(titlebar()->height(), titlebar()->height()));
    });
#endif

    checkForUpdateLogs();
}

void ChatWindow::showWindowMode()
{
    setWindowIconVisible(true);
    setWindowTitleVisible(m_hasChatHistory);
    setTitlebarShadowEnabled(true);
    //setAttribute(Qt::WA_TranslucentBackground, false);
    //setAttribute(Qt::WA_NoSystemBackground, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(m_wWindowFlags);
    qCInfo(logAIGUI)<<"last time window size:"<<m_windowSize;
    qCInfo(logAIGUI)<<"now window size:"<<this->size();

    // Clear any fixed size constraints from sidebar mode
    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    activeShowWindow();
    updateSystemTheme();
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    DbWrapper::localDbWrapper().updateDisplayMode(WINDOW_MODE);

    setDigitalImageDisable(m_isDigitalImageDisable);
}

void ChatWindow::showSidebarMode()
{
    setWindowIconVisible(m_hasChatHistory);
    setWindowTitleVisible(m_hasChatHistory);
    setTitlebarShadowEnabled(false);
    setEnableBlurWindow(true);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(m_sWindowFlags);
    updateSystemTheme();
    activeShowWindow();

    DbWrapper::localDbWrapper().updateDisplayMode(SIDEBAR_MODE);

    setDigitalImageDisable(m_isDigitalImageDisable);
}

void ChatWindow::searchShortCut()
{
    qCInfo(logAIGUI) << "Search application shortcuts";
    ShortcutManager& shortcutMgr = ShortcutManager::getInstance();

    if (!shortcutMgr.isValid()) {
        qCWarning(logAIGUI) << "ShortcutManager D-Bus interface is not valid";
        return;
    }

    // 查询现有快捷键
    QList<ShortcutInfo> shortcuts = shortcutMgr.searchShortcuts("UOS AI");
    ShortcutInfo uosAiShortcut;

    for (const ShortcutInfo &shortcut : shortcuts) {
        if (shortcut.id == "UOS AI") {
            uosAiShortcut = shortcut;
        }
    }

    m_currentShortcut = uosAiShortcut.accel;
}

void ChatWindow::setW(int w)
{
    setFixedWidth(w);
    move(m_geometry.x() + DefaultMargin + m_geometry.width() - w, m_geometry.y());
}

void ChatWindow::setX(int x)
{
    move(x, m_geometry.y());
}

void ChatWindow::activeShowWindow()
{
    int changeTime = 100;
#ifdef Q_PROCESSOR_LOONGARCH_64
    changeTime = 500;
#endif
    QTimer::singleShot(changeTime, this, [this]() {
        qCInfo(logAIGUI) << "activeShowWindow - current size:" << this->size() << "normal size:" << m_windowSize << "maximized:" << m_isMaximized;
        if (m_displayMode == WINDOW_MODE) {
            if ((this->size() != m_windowSize || !m_windowSize.isEmpty()) && !m_isMaximized) {
                qCInfo(logAIGUI) << "Resizing to normal size:" << m_windowSize;
                resize(m_windowSize);
                // Force immediate geometry update
                QCoreApplication::processEvents();
                qCInfo(logAIGUI) << "Resizing size:" << this->window()->size();
            }
            setMinimumSize(1010, 750);
            setMaximumSize(4096, 4096);
        } else {
            updateSidebarGeometry();
        }

        if (m_isMaximized && m_displayMode == WINDOW_MODE) {
            showMaximized();
        } else {
            showNormal();
        }

        if (m_needActiveWindow) {
            activateWindow();
            this->setFocus();
            m_webView->setFocus();
        }
        m_needActiveWindow = true;

        if (m_displayMode == WINDOW_MODE && !m_isMaximized) {
            Dtk::Widget::moveToCenter(this);
        }
    });
}

QString ChatWindow::getLatestUpdateLogVersion()
{
    QString filePath {};
    if (Util::checkLanguage()) {
        filePath = ":/assets/updatelog/updatelog-zh_CN.json";
    } else {
        filePath = ":/assets/updatelog/updatelog-en_US.json";
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(logAIGUI) << "Failed to open updatelog file:" << filePath;
        return QString();
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isArray()) {
        qCWarning(logAIGUI) << "Invalid updatelog JSON format:" << filePath;
        return QString();
    }

    QJsonArray logArray = doc.array();
    if (logArray.isEmpty()) {
        return QString();
    }

    // 是按时间倒序排列的，第一个条目是最新的
    QJsonObject latestLog = logArray[0].toObject();
    return latestLog["version"].toString();
}

void ChatWindow::checkForUpdateLogs()
{
    // 1. 读取 updatelog JSON 文件中的最新版本号
    QString latestVersion = getLatestUpdateLogVersion();
    if (latestVersion.isEmpty()) {
        return;
    }

    // 2. 从 ConfigTable 中读取 ShownUpdateLogVersion
    auto confValue = ConfigTable::get(ConfigTable::ShownUpdateLogVersion);
    QString shownVersion = confValue.value();

    // 3. 比较版本号
    bool hasUpdate = (latestVersion != shownVersion);

    // 更新更新日志菜单项的红点
    DStyle::setRedPointVisible(m_updateLogAction, hasUpdate);

    // 通过统一的红点管理更新 OptionButton 的红点状态
    setRedPointSource(UpdateLogSource, hasUpdate);
}


void ChatWindow::onModalStateChanged(bool modal)
{
    qCInfo(logAIGUI) << "Modal state changed:" << modal;
    updateSystemTheme();

    m_webView->setModalState(modal);
}

void ChatWindow::onMenuTriggered(QAction *action)
{
    if (action == m_windowModeAction) {
#ifdef COMPILE_ON_V25
        if (ESystemContext::isTreeland()) {
            close();
        }
#endif
        m_windowModeAction->setChecked(true);
        m_displayMode = WINDOW_MODE;
        qCInfo(logAIGUI) << "Switching to window mode";

        m_webView->setWindowMode(true);
        ServerWrapper::instance()->updateVisibleState(false);
#ifdef COMPILE_ON_V25
        QTimer::singleShot(0, [this]() {
            showWindowMode();
        });
#else
       showWindowMode();
#endif
    } else if (action == m_sidebarModeAction) {
#ifdef COMPILE_ON_V25
        if (ESystemContext::isTreeland()) {
            close();
        }
#endif
        m_sidebarModeAction->setChecked(true);
        m_displayMode = SIDEBAR_MODE;
        qCInfo(logAIGUI) << "Switching to sidebar mode";

        m_webView->setWindowMode(false);
        ServerWrapper::instance()->updateVisibleState(true);
#ifdef COMPILE_ON_V25
        QTimer::singleShot(0, [this]() {
            showSidebarMode();
        });
#else
       showSidebarMode();
#endif
    } else if(action == m_settingsAction) {
        qCInfo(logAIGUI) << "Opening LLM config window";
        EAiExec()->launchLLMConfigWindow(false, AssistantType::PLUGIN_ASSISTANT == EAiExec()->currentAssistantType());
    } else if(action == m_updateLogAction) {  // 新增更新记录处理
        qCInfo(logAIGUI) << "Opening update log window";

        // 更新 ConfigTable 中的 ShownUpdateLogVersion
        QString latestVersion = getLatestUpdateLogVersion();
        if (!latestVersion.isEmpty()) {
            auto shownVersion = ConfigTable::get(ConfigTable::ShownUpdateLogVersion);
            if (shownVersion.value().isEmpty()) {
                shownVersion.setName("ShownUpdateLogVersion");
                shownVersion.setDesc("");
                shownVersion.setType(ConfigTable::ShownUpdateLogVersion);
                shownVersion.setValue(latestVersion);
                shownVersion.save();
            } else {
                shownVersion.setValue(latestVersion);
                shownVersion.update();
            }
        }

        // 隐藏小红点
        DStyle::setRedPointVisible(m_updateLogAction, false);
        setRedPointSource(UpdateLogSource, false);

        uos_ai::UpdateLogDialog dialog;
        dialog.exec();
    }
}

void ChatWindow::onChatBtnClicked()
{
    if (m_isDigitalMode) {
        m_webView->setToChatMode();
        m_webView->setFocus();
        if (m_displayMode == WINDOW_MODE)
            setWindowIconVisible(true);

        if (m_hasChatHistory) {
            setWindowIconVisible(true);
            setWindowTitleVisible(true);
        }
        qCInfo(logAIGUI) << "Switching to chat mode";
    } else {
        m_webView->setToDigitalMode();
        m_webView->setFocus();
        if (m_displayMode == SIDEBAR_MODE)
            setWindowIconVisible(false);
        setWindowTitleVisible(false);
        qCInfo(logAIGUI) << "Switching to digital mode";
    }

    m_isDigitalMode = !m_isDigitalMode;
    m_chatBtn->updateActiveStatus(m_isDigitalMode);
}

void ChatWindow::digital2ChatStatusChange()
{
    // Digital To Chat Status Change
    if (m_displayMode == WINDOW_MODE)
        setWindowIconVisible(true);

    if (m_hasChatHistory) {
        setWindowIconVisible(true);
        setWindowTitleVisible(true);
    }

    m_isDigitalMode = !m_isDigitalMode;
    m_chatBtn->updateActiveStatus(m_isDigitalMode);
}

void ChatWindow::previewReferenceDoc(const QString &docPath, const QStringList &docContents)
{
    if (m_knowBaseDialog) {
        m_knowBaseDialog->close();
    }

    m_knowBaseDialog = new uos_ai::ReferenceDialog(docPath, docContents, this);
    m_knowBaseDialog->show();

    connect(m_knowBaseDialog, &uos_ai::ReferenceDialog::finished, this, [&]{
        m_knowBaseDialog->deleteLater();
        m_knowBaseDialog = nullptr;
    });
}

void ChatWindow::overrideQuestion(const QString &question, const QMap<QString, QString> &ext)
{
    // DBus接口使用
    // 前端准备工作
    if (!m_webView->chatInitFinished()) {
        if (!EAsync()->chatInitSyncWorker())
            return;

        if (!EAsync()->executeSynchronouslyGeneric(EAsyncWorker::OverrideInput))
            return;
    } else {
        if (!EAsync()->executeSynchronouslyGeneric(EAsyncWorker::OverrideInput))
            return;
    }

    if (m_isDigitalMode) {
        m_webView->setToChatMode();
        if (m_displayMode == WINDOW_MODE)
            setWindowIconVisible(true);

        if (m_hasChatHistory) {
            setWindowIconVisible(true);
            setWindowTitleVisible(true);
        }

        m_isDigitalMode = !m_isDigitalMode;
        m_chatBtn->updateActiveStatus(m_isDigitalMode);
    }
    this->setFocus();
    m_webView->setFocus();

    if (ext.contains("file")) {
        QString filePath      = ext["file"];
        QString defaultPrompt = ext["defaultPrompt"];
        qCInfo(logAIGUI) << "question:" << question << "defaultPrompt:" << defaultPrompt << "file:" << filePath;

        if (m_webView->chatInitFinished()) {
            // 传入文档
            EAiExec()->onDocSummaryDragInView(QStringList(filePath), defaultPrompt);
        } else {
            QVariantList params {filePath, defaultPrompt};
            m_pendingTasks << qMakePair(ChatInitAsyncTask::ParserDocument, params);
        }
    }

    if (m_webView->chatInitFinished()) {
        // 问题追加
        emit EAiExec()->sigOverrideQues(question);
    } else {
        QVariantList params {question};
        m_pendingTasks << qMakePair(ChatInitAsyncTask::OverrideQuestion, params);
    }
}

void ChatWindow::appendQuestion(const QString &question)
{
    if (!EAsync()->executeSynchronouslyGeneric(EAsyncWorker::AppendPrompt))
        return;

    if (m_isDigitalMode) {
        m_webView->setToChatMode();
        if (m_displayMode == WINDOW_MODE)
            setWindowIconVisible(true);

        if (m_hasChatHistory) {
            setWindowIconVisible(true);
            setWindowTitleVisible(true);
        }

        m_isDigitalMode = !m_isDigitalMode;
        m_chatBtn->updateActiveStatus(m_isDigitalMode);
    }
    this->setFocus();
    m_webView->setFocus();

    if (m_webView->chatInitFinished()) {
        // 问题追加
        emit EAiExec()->sigOverrideQues(question);
    } else {
        QVariantList params {question};
        m_pendingTasks << qMakePair(ChatInitAsyncTask::OverrideQuestion, params);
    }
}

void ChatWindow::appendImage(const QString &imagePath)
{
    if (!EAsync()->executeSynchronouslyGeneric(EAsyncWorker::AppendImage))
        return;
    if (m_isDigitalMode) {
        m_webView->setToChatMode();
        if (m_displayMode == WINDOW_MODE)
            setWindowIconVisible(true);

        if (m_hasChatHistory) {
            setWindowIconVisible(true);
            setWindowTitleVisible(true);
        }

        m_isDigitalMode = !m_isDigitalMode;
        m_chatBtn->updateActiveStatus(m_isDigitalMode);
    }
    this->setFocus();
    m_webView->setFocus();

    if (m_webView->chatInitFinished()) {
        // 传入图片
        EAiExec()->onDocSummaryDragInView(QStringList(imagePath), QString());
    } else {
        QVariantList params {imagePath, QString()};
        m_pendingTasks << qMakePair(ChatInitAsyncTask::ParserDocument, params);
    }
}

void ChatWindow::appendAskQuestion(int assistantType)
{
    switch (assistantType) {
    case AssistantType::UOS_AI:
    {
        if (!EAsync()->executeSynchronouslyGeneric(EAsyncWorker::ChangeToUosAI))
            return;
        break;
    }

    default:
        return;
    }

    if (m_isDigitalMode) {
        m_webView->setToChatMode();
        if (m_displayMode == WINDOW_MODE)
            setWindowIconVisible(true);

        if (m_hasChatHistory) {
            setWindowIconVisible(true);
            setWindowTitleVisible(true);
        }

        m_isDigitalMode = !m_isDigitalMode;
        m_chatBtn->updateActiveStatus(m_isDigitalMode);
    }
    this->setFocus();
    m_webView->setFocus();

    if (m_webView->chatInitFinished()) {
        // 传入问题
        EAiExec()->appendWordWizardQuestion(assistantType);
    } else {
        QVariantList params {assistantType};
        m_pendingTasks << qMakePair(ChatInitAsyncTask::AddAskQuestion, params);
    }
}

void ChatWindow::addKnowledgeBase(const QStringList &knowledgeBasefile)
{
    m_needActiveWindow = false;
    if (m_webView->chatInitFinished()) {
        emit sigToAddKnowledgeBase(knowledgeBasefile);
    } else {
        QVariantList params {knowledgeBasefile};
        m_pendingTasks << qMakePair(ChatInitAsyncTask::AddKnowledgeBase, params);
    }
}

void ChatWindow::setTitleBarStatus(bool status)
{
    titlebar()->setDisabled(status);
}

bool ChatWindow::showWarningDialog(const QString assistantId, const QString conversationId,  const QString msg, bool isDelete, bool isLlmDelete, bool isAllConvDelete)
{
    qCInfo(logAIGUI) << "Showing warning dialog for assistant:" << assistantId << "conversation:" << conversationId;
    DDialog dlg(this);
    dlg.setMinimumWidth(380);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    if(!isDelete && !isLlmDelete){  //智能体删除或清空历史记录的情况
        QStringList list = msg.split("_&_");
        dlg.setTitle(list.first());
        dlg.setMessage(list.last());
    }else {
        dlg.setMessage(QString(msg));
    }

    if(isDelete || isAllConvDelete){
        dlg.addButton(tr("Cancel"), true, DDialog::ButtonNormal);
        dlg.addButton(tr("Delete"), true, DDialog::ButtonWarning);
    }else {
        dlg.addButton(tr("Confirm"), true, DDialog::ButtonNormal);
    }

    switch (dlg.exec()) {
    case -1:
    {
        if(isLlmDelete){
            emit EAiExec()->sigHideHistoryList();
            setTitleBarStatus(false);
        }
        return false;
    }

    case 0:
    {
        if(isLlmDelete){
            emit EAiExec()->sigHideHistoryList();
            setTitleBarStatus(false);
        }
        return false;
    }
    case 1:
    {
        if (isAllConvDelete) {
            EAiExec()->removeAllConversation();
        } else {
            EAiExec()->removeConversation(assistantId, conversationId);
        }
        emit EAiExec()->sigGetNewHistoryList(conversationId);
    }

    }
    return true;
}

bool ChatWindow::showRmMcpServerDlg(const QString &name)
{
    qCDebug(logAIGUI) << "Showing remove dialog for MCP server:" << name;
    DDialog dlg(this);
    dlg.setMinimumWidth(380);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.setTitle(QString(tr("Confirm deletion %1?")).arg(name));
    dlg.setMessage(tr("After deletion, this server will be unavailable. Proceed with caution."));
    dlg.addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    dlg.addButton(tr("Delete"), true, DDialog::ButtonWarning);
    if (dlg.exec() == DDialog::Accepted) {
        qCInfo(logAIGUI) << "Remove MCP server:" << name;
        return true;
    }

    return false;
}

void ChatWindow::showUpdateDialog(const QString &msg, const QString &appName)
{
    DDialog dlg(this);
    dlg.setMinimumWidth(380);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.setMessage(QString(msg));
    dlg.addButton(tr("Cancel"), true, DDialog::ButtonNormal);
    dlg.addButton(tr("Update"), true, DDialog::ButtonRecommend);
    if (dlg.exec() == DDialog::Accepted) {
        qCInfo(logAIGUI) << "Need update app:" << appName;
        EAiExec()->openInstallWidget(appName);
    }
}

bool ChatWindow::showRemoveFileDialog(const QString &message)
{
    DDialog dlg(this);
    dlg.setMinimumWidth(380);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.setTitle(message);
    dlg.addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    dlg.addButton(tr("Delete"), true, DDialog::ButtonWarning);
    if (dlg.exec() == DDialog::Accepted) {
        return true;
    }

    return false;
}

bool ChatWindow::showAllowUploadFilesAlert(int modelType, const QString &modelDisplayName, bool searchOnline)
{
    // 判断是否为本地模型
    QList<LLMChatModel> localModelList;
    localModelList << LLMChatModel::LOCAL_OTHER_MODEL
                   << LLMChatModel::LOCAL_TEXT2IMAGE
                   << LLMChatModel::LOCAL_YOURONG_1_5B
                   << LLMChatModel::LOCAL_YOURONG_7B;

    UploadFilesAlertDialog dialog;
    bool curLLMIsOnline = !localModelList.contains(static_cast<LLMChatModel>(modelType));
    if (curLLMIsOnline) {
        // 情况一：使用线上模型
        QString modelName;
        if (static_cast<LLMChatModel>(modelType) == OPENAI_API_COMPATIBLE) {
            modelName = modelDisplayName;
        } else {
            modelName = LLMServerProxy::llmName(static_cast<LLMChatModel>(modelType));
        }
        dialog.showOnlineModelAlert(modelName);
    } else if (searchOnline) {
        // 情况二：本地模型 + 联网搜索
        dialog.showLocalModelAlert();
    } else {
        // 其他情况直接返回 true，不弹窗
        return true;
    }

    if (dialog.exec() == DDialog::Accepted) {
        return true;
    }

    return false;
}

void ChatWindow::startScreenshot()
{
    emit EAiExec()->sigStartScreenshot();

}

bool ChatWindow::getThirdPartyMcpAgreement()
{
    //先查询数据库，没同意就弹窗
    if(DbWrapper::localDbWrapper().getThirdPartyMcpAgreement()){
        qCDebug(logAIGUI) << "The tripartite MCP server agreement is now agreed upon." ;
        return true;
    }

    ECheckAgreementDialog dlg;
    dlg.exec();
    bool agreed = DbWrapper::localDbWrapper().getThirdPartyMcpAgreement();

    if (agreed) {
        emit sigThirdPartyMcpAgree();
    }

    return agreed;
}

void ChatWindow::setNeedShowLLMConfigWindow(bool isNeedShowLLMConfigWindow)
{
    s_isNeedShowLLMConfigWindow = isNeedShowLLMConfigWindow;
}

void ChatWindow::showKnowledgeBaseErrorDialog(int type, QString appName)
{
    QString title;
    QString msg;
    DDialog dlg(this);
    dlg.setMinimumWidth(380);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.addButton(tr("Cancel"), true, DDialog::ButtonNormal);
    if(type == 0){
        title = tr("Non-vectorized plugin");
        msg = tr("Before using the [AI Knowledge Base], you need to install the vectorization plugin first, so that the AI knowledge base function can work properly.");
        dlg.addButton(tr("Install"), true, DDialog::ButtonRecommend);
    }else {
        title = tr("The knowledge base is empty");
        msg = tr("Before using the [AI Knowledge Base], you need to first add documents to the knowledge base. After adding, the AI will answer questions based on the content you have added to the knowledge base.");
        dlg.addButton(tr("Add Files"), true, DDialog::ButtonRecommend);
    }
    dlg.setTitle(QString(title));
    dlg.setMessage(QString(msg));
    if (dlg.exec() == DDialog::Accepted) {
        if (type == 0){
            // 去安装向量化插件
            EAiExec()->openInstallWidget(appName);
            qCDebug(logAIGUI) << "To install " << appName;
        }else {
           // 去配置知识库
            EAiExec()->launchKnowledgeBaseConfigWindow();
            qCDebug(logAIGUI) << "To add knowledge base file." ;
        }
    }
}

bool ChatWindow::showLostFileWarningDlg(const QString lostFileList)
{
    qCWarning(logAIGUI) << "showLostFileWarningDlg lostFileList:" << lostFileList;
    
    QJsonDocument jsonDoc = QJsonDocument::fromJson(lostFileList.toUtf8());
    if (!jsonDoc.isArray()) {
        qCWarning(logAIGUI) << "Invalid JSON array format";
        return false;
    }

    DDialog dlg(this);
    dlg.setMinimumWidth(380);
    dlg.setTitle(QString(tr("The following file has expired and cannot be used. Continue?")));
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));

    dlg.addSpacing(20);

    // Create a container widget for all file items, set dlg as parent to manage memory
    QWidget *containerWidget = new QWidget(&dlg);
    QVBoxLayout *containerLayout = new QVBoxLayout(containerWidget);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    QJsonArray jsonArray = jsonDoc.array();
    for (const QJsonValue &value : jsonArray) {
        QJsonObject obj = value.toObject();
        QString fileNameText = obj["fileNameText"].toString();
        QString imgBase64 = obj["imgBase64"].toString();

        // Create widget for each file item, set containerWidget as parent
        QWidget *fileWidget = new QWidget(containerWidget);
        QHBoxLayout *fileLayout = new QHBoxLayout(fileWidget);
        fileLayout->setContentsMargins(81, 0, 81, 10);

        // Create icon label
        QLabel *iconLabel = new QLabel(fileWidget);
        iconLabel->setFixedSize(16, 16);
        
        // Decode base64 image
        if (!imgBase64.isEmpty()) {
            QByteArray imageData = QByteArray::fromBase64(imgBase64.toUtf8());
            QPixmap pixmap;
            if (pixmap.loadFromData(imageData)) {
                // 获取设备像素比
                qreal devicePixelRatio = qApp->devicePixelRatio();
                
                // 计算缩放后的尺寸，考虑设备像素比
                int scaledSize = static_cast<int>(16 * devicePixelRatio);
                
                // 先对pixmap进行缩放
                QPixmap scaledPixmap = pixmap.scaled(scaledSize, scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                
                // 设置缩放系数
                scaledPixmap.setDevicePixelRatio(devicePixelRatio);
                
                // 设置到label中
                iconLabel->setPixmap(scaledPixmap);
            } else {
                // No valid image data, keep as empty placeholder
                qCWarning(logAIGUI) << "Failed to load image from base64 data from file:" << fileNameText;
            }
        } else {
            // No base64 data, keep as empty placeholder
            qCWarning(logAIGUI) << "No base64 image data provided from file " << fileNameText;
        }

        // Create filename label with elided text
        QLabel *fileNameLabel = new QLabel(fileWidget);
        fileNameLabel->setText(fileNameText);
        
        // Set elide mode to show ... in the middle
        QFontMetrics fm(fileNameLabel->font());
        int maxWidth = 300; // Maximum width for filename
        QString elidedText = fm.elidedText(fileNameText, Qt::ElideMiddle, maxWidth);
        fileNameLabel->setText(elidedText);
        fileNameLabel->setToolTip(fileNameText); // Show full name on hover

        fileLayout->addWidget(iconLabel);
        fileLayout->addWidget(fileNameLabel);
        fileLayout->addStretch(); // Push content to left
        containerLayout->addWidget(fileWidget);
    }

    dlg.addSpacing(10);
    dlg.addContent(containerWidget);
    dlg.addButton(tr("Cancel"), true, DDialog::ButtonNormal);
    dlg.addButton(tr("Confirm"), true, DDialog::ButtonRecommend);

    if (dlg.exec() == DDialog::Accepted) {
        return true;
    }
    return false;
}

bool ChatWindow::showInstallUosAIAgentDlg(QString appName)
{
    DDialog dlg(this);
    dlg.setMinimumWidth(380);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.addButton(tr("Use later"), true, DDialog::ButtonNormal);
    dlg.addButton(tr("Install Now"), true, DDialog::ButtonRecommend);

    dlg.setTitle(QString(tr("MCP & Skills environment missing")));
    dlg.setMessage(QString(tr("Please go to the App Store to install UOS AI Agent")));
    if (dlg.exec() == DDialog::Accepted) {
        EAiExec()->openInstallWidget(appName);
    }
    return true;
}

int ChatWindow::getTitleBarBtnWidth()
{
    auto closeBtn = titlebar()->findChild<QAbstractButton *>("DTitlebarDWindowCloseButton");
    if (closeBtn){
        return closeBtn->width();
    }else {
        return 50;
    }
}

bool ChatWindow::showGetFreeCreditsDlg()
{
    DDialog dlg(this);
    dlg.setWindowFlags(dlg.windowFlags() | Qt::Tool | Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint);
#ifdef COMPILE_ON_V25
    dlg.setMaximumWidth(380);
    dlg.setMinimumHeight(180);
#else
    dlg.setMinimumWidth(380);
#endif
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.addButton(tr("Not Now"), true, DDialog::ButtonNormal);
    dlg.addButton(tr("Claim Credits"), true, DDialog::ButtonRecommend);

    dlg.setTitle(QString(tr("Free Credits Delivered")));
    dlg.setMessage(QString(tr("You've used up the free generation credits for your trial account. We've given you an extra 200 free credits valid this month. Explore more features and unlock UOS AI's limitless capabilities!")));
    if (dlg.exec() == DDialog::Accepted) {
        return true;
    }
    return false;
}

QString ChatWindow::getCurrentShortcut()
{
    return m_currentShortcut;
}

bool ChatWindow::isDeleteOutlineTitle()
{
    DDialog dlg(this);
    dlg.setMinimumWidth(380);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.addButton(tr("Cancel"), true, DDialog::ButtonNormal);
    dlg.addButton(tr("Delete"), true, DDialog::ButtonWarning);
    dlg.setTitle(QString(tr("Delete this title?")));  // 是否删除该标题
    if (dlg.exec() == DDialog::Accepted) {
        return true;
    }
    return false;
}

void ChatWindow::iconThemeChanged()
{
    emit EAiExec()->sigIconThemeChanged();
}

void ChatWindow::setDigitalImageDisable(bool disable)
{
    m_isDigitalImageDisable = disable;
    
    // 综合考虑所有状态因素来设置按钮的禁用状态
    bool shouldDisable = m_isDigitalImageDisable || m_voiceConversationDisabled;
    
    // 检查语音对话状态
    if (m_voiceConversationStatus == 3 || m_voiceConversationStatus == 4) {
        shouldDisable = true;
    }

    m_chatBtn->setDisabled(shouldDisable);
}

bool ChatWindow::isActiveChatFromDigitalImage()
{
    bool ret = m_isActiveChatFromDigitalImage;
    m_isActiveChatFromDigitalImage = false;
    return ret;
}

void ChatWindow::onVoiceConversationStatusChanged(int status)
{
//    if (m_voiceConversationStatus == status)
//        return;

    m_voiceConversationStatus = status;

    switch (m_voiceConversationStatus) {
    case 2:
        titlebar()->setMenuDisabled(true);
        break;
    case 3:
    case 4:
        m_chatBtn->setDisabled(true);
        break;
    case 1:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    default:
        m_chatBtn->setDisabled(false);
        titlebar()->setMenuDisabled(false);
        break;

    }

}

void ChatWindow::onGenPersonalFAQ()
{
    EAiExec()->personalFAQGenerate();
}

void ChatWindow::onDocSummaryDragInView(const QStringList &docPaths)
{
    EAiExec()->onDocSummaryDragInView(docPaths, QString());
}

void ChatWindow::onChatInitFinished()
{
    //使用 std::call_once 确保 ShortcutUpdateDialog::checkAndShow 只调用一次
    std::call_once(m_shortcutUpdateDialogOnceFlag, [this](){
        //延迟保证前端页面渲染完毕
        QTimer::singleShot(0, this, [this]{
            qCInfo(logAIGUI) << "Check if the update prompt box is displayed";
            ShortcutUpdateDialog::checkAndShow(this);
        });
    });

    //延迟保证先弹提示框再处理任务
    QTimer::singleShot(0, this, [this]{
        qCInfo(logAIGUI) << "Front-end page rendering is complete, starting asynchronous tasks.";
        emit EAsync()->sigChatInitSyncWorkerFinished();

        for (AiTask task : m_pendingTasks) {
            int type = task.first;
            QVariantList params = task.second;
            switch (type) {
            case ChatInitAsyncTask::ParserDocument:
                if (params.size() == 2)
                    EAiExec()->onDocSummaryDragInView(QStringList(params[0].toString()), params[1].toString());
                break;
            case ChatInitAsyncTask::OverrideQuestion:
                if (params.size() == 1) {
                    emit EAiExec()->sigOverrideQues(params[0].toString());
                }
                break;
            case ChatInitAsyncTask::AddKnowledgeBase:
                if (params.size() == 1) {
                    emit sigToAddKnowledgeBase(params[0].toStringList());
                }
                break;
            case ChatInitAsyncTask::AddAskQuestion:
                if (params.size() == 1) {
                    EAiExec()->appendWordWizardQuestion(params[0].toInt());;
                }
                break;
            }
        }
        m_pendingTasks.clear();
    });

    // 延迟保证先绘制完chatwindow，再弹设置窗口
    if (s_isNeedShowLLMConfigWindow) {
        QTimer::singleShot(0, this, [this]{
            EAiExec()->launchLLMConfigWindow(false, AssistantType::PLUGIN_ASSISTANT == EAiExec()->currentAssistantType());
        });
        setNeedShowLLMConfigWindow(false);
    }
}

void ChatWindow::onRedPointVisible(bool isVisible)
{
    DStyle::setRedPointVisible(m_settingsAction, isVisible);
    setRedPointSource(SettingsSource, isVisible);
}

void ChatWindow::updateSidebarGeometry(int currentWidth)
{
#ifdef COMPILE_ON_V23    
    int mode = 0;
#else
    int mode = m_dock->displayMode();
#endif
    qCInfo(logAIGUI)<<"sidebar currentWidth: "<<currentWidth;
    DockPosition dockPosition = static_cast<DockPosition>(m_dock->position());
    qCInfo(logAIGUI)<<"dockPosition: "<<dockPosition;
    int dockMargin = 0;
    qreal ratio = qApp->primaryScreen()->devicePixelRatio();
    QRect frontendWindowRect = m_dock->frontendWindowRect();
    qCInfo(logAIGUI)<<"frontendWindowRect: "<<frontendWindowRect;

    m_displayRect = QGuiApplication::primaryScreen()->geometry();
    qCInfo(logAIGUI)<<"primaryScreen geometry: "<<QGuiApplication::primaryScreen()->geometry();
    QList<QScreen*> screenList = QGuiApplication::screens();
    for (const QScreen *screen : screenList) {
        QRect screenRect = QRect(screen->geometry().x(), screen->geometry().y(),
                                 screen->geometry().width()*ratio, screen->geometry().height()*ratio);
        qCInfo(logAIGUI)<<"screenRect: "<<screenRect;
        if (screenRect.contains(frontendWindowRect.center())) {
            m_displayRect = screen->geometry();
            break;
        }
    }
    qCInfo(logAIGUI)<<"Current displayRect: "<<m_displayRect;

    int width = currentWidth;
    int height = m_displayRect.height() - DefaultMargin * 2;

    if (dockPosition == DockPosition::Top || dockPosition == DockPosition::Bottom) {
        if (mode == DockModel::Fashion) {
            height = m_displayRect.height() - DefaultMargin * 2 - frontendWindowRect.height() / ratio - dockMargin;
        } else {
            height = m_displayRect.height() - DefaultMargin * 2 - frontendWindowRect.height() / ratio;
        }
    }

    int x = m_displayRect.x() + m_displayRect.width() - currentWidth - DefaultMargin;
    if (dockPosition == DockPosition::Right) {
        if (mode == DockModel::Fashion) {
            x =  m_displayRect.x() + m_displayRect.width() - (currentWidth + frontendWindowRect.width() / ratio + dockMargin + DefaultMargin);
        } else {
            x =  m_displayRect.x() + m_displayRect.width() - (currentWidth + frontendWindowRect.width() / ratio + DefaultMargin);
        }
    }

    int y = m_displayRect.y() + DefaultMargin;
    if (dockPosition == DockPosition::Top) {
        if (mode == DockModel::Fashion) {
            y = m_displayRect.y() + DefaultMargin + frontendWindowRect.height() / ratio + dockMargin;
        } else {
            y = m_displayRect.y() + DefaultMargin + frontendWindowRect.height() / ratio;
        }
    }

    m_geometry = QRect(x, y, width, height);
    qCInfo(logAIGUI)<<"sidebar geometry: "<<m_geometry;
    setFixedHeight(height);
    setMaximumWidth(m_displayRect.width() / 2 - DefaultMargin);
    setMinimumWidth(400);

#ifdef COMPILE_ON_V25
    if (ESystemContext::isTreeland()) {
        resize(m_geometry.size());
        if (windowHandle() != nullptr) {
            auto layerShellWnd = DLayerShellWindow::get(windowHandle());

            layerShellWnd->setAnchors({ DLayerShellWindow::AnchorRight,
                                       DLayerShellWindow::AnchorBottom,
                                       DLayerShellWindow::AnchorTop });
            int defaultMargin = DefaultMargin;
            layerShellWnd->setTopMargin(defaultMargin);
            layerShellWnd->setBottomMargin(defaultMargin);
            layerShellWnd->setRightMargin(defaultMargin);

            layerShellWnd->setLayer(DLayerShellWindow::LayerOverlay);
            layerShellWnd->setKeyboardInteractivity(DLayerShellWindow::KeyboardInteractivityOnDemand);
        }
        return;
    }
#endif
    setGeometry(m_geometry);
}

void ChatWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_displayMode == SIDEBAR_MODE) {
        Q_UNUSED(event);
        return;
    } else {
        return DMainWindow::mouseMoveEvent(event);
    }
}

void ChatWindow::hideEvent(QHideEvent *event)
{
    m_webView->setTohiddenState();

    if (m_displayMode == SIDEBAR_MODE)
        ServerWrapper::instance()->updateVisibleState(false);

    return DMainWindow::hideEvent(event);
}

void ChatWindow::showEvent(QShowEvent *event)
{
    m_webView->setToShowedState();

    if (m_displayMode == SIDEBAR_MODE)
        ServerWrapper::instance()->updateVisibleState(true);

    return DMainWindow::showEvent(event);
}

void ChatWindow::resizeEvent(QResizeEvent *event)
{
    if (m_displayMode == WINDOW_MODE) {
        bool wasMaximized = m_isMaximized;
        m_isMaximized = isMaximized();
        
        if (!m_isMaximized) {
            // 窗口处于正常状态，更新正常窗口尺寸
            // 只有在窗口不是从最大化状态恢复时才更新正常尺寸
            // 这样可以避免在恢复过程中多次触发尺寸更新
            if (!wasMaximized) {
                m_windowSize = event->size();
                QString sizeStr = QString::number(m_windowSize.width()) + "#" + QString::number(m_windowSize.height());
                DbWrapper::localDbWrapper().updateWindowSize(sizeStr);
                qCInfo(logAIGUI)<<"m_windowSize changed to: "<<m_windowSize;
            } else {
                // Window was restored from minimized/maximized state
                qCInfo(logAIGUI)<<"Window restored, setting size to: "<<m_windowSize;
                if (m_windowSize != event->size()) {
                    resize(m_windowSize);
                }
            }
        } else if (!wasMaximized && m_isMaximized) {
            // 窗口刚从正常状态变为最大化状态，保存正常状态下的尺寸
            QString sizeStr = QString::number(m_windowSize.width()) + "#" + QString::number(m_windowSize.height());
            DbWrapper::localDbWrapper().updateWindowSize(sizeStr);
            qCInfo(logAIGUI)<<"Window maximized, normal size saved: "<<m_windowSize;
        }
    }
    return DMainWindow::resizeEvent(event);
}

void ChatWindow::paintEvent(QPaintEvent *pe)
{
    //    QPainter painter(this);
    //    painter.setRenderHint(QPainter::Antialiasing, true);
    //    painter.setPen(m_backgroundColor);
    //    painter.setBrush(QBrush(m_backgroundColor));
    //    painter.drawRect(rect());

    return DMainWindow::paintEvent(pe);
}

#ifdef COMPILE_ON_QT6
bool ChatWindow::event(QEvent *event)
{
    bool ret = DMainWindow::event(event);
    if (event->type() == QEvent::ApplicationFontChange) {
        QMetaObject::invokeMethod(qApp, "fontChanged", Qt::QueuedConnection, Q_ARG(QFont, this->font()));
    }
    return ret;
}
#endif

bool ChatWindow::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    switch (event->type()) {
    case QEvent::WindowActivate: {
        if (m_webView) {
            m_webView->setWindowActiveState(true);
            this->setFocus();
            m_webView->setFocus();
            emit EAiExec()->sigInputFocus();
        }
        if (m_displayMode == SIDEBAR_MODE)
            ServerWrapper::instance()->updateActiveState(true);
    } break;
    case QEvent::WindowDeactivate: {
        if (m_webView) {
            m_webView->setWindowActiveState(false);
        }
        if (m_displayMode == SIDEBAR_MODE)
            ServerWrapper::instance()->updateActiveState(false);
    } break;
    default:
        break;
    };
    return DMainWindow::eventFilter(obj, event);
}

void ChatWindow::updateSystemTheme()
{
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    QPalette pa = titlebar()->palette();
    pa.setColor(QPalette::Base, Qt::transparent);

    //不透明度设置下限
    if(m_alpha < m_minAlpha){
        m_alpha = m_minAlpha;
    }

    int alpha = 255;
    if (m_displayMode == SIDEBAR_MODE && m_hasBlurWindow) {
        alpha = m_alpha * 255;

        if (alpha < 0 || alpha > 255 )
            alpha = 160;
    }

    if (themeType == DGuiApplicationHelper::DarkType) {
        m_backgroundColor = QColor(37, 37, 37, alpha);

        if (m_displayMode == SIDEBAR_MODE && m_hasBlurWindow) {
            m_backgroundColor = QColor(0, 0, 0, alpha);
        }

        pa.setColor(QPalette::Base, m_backgroundColor);
    } else {
        m_backgroundColor = QColor(248, 248, 248, alpha);

        if (m_displayMode == SIDEBAR_MODE && m_hasBlurWindow) {
            m_backgroundColor = QColor(255, 255, 255, alpha);
        }

        pa.setColor(QPalette::Base, m_backgroundColor);
    }

    QString backgroundColor = QString("rgba(%1, %2, %3, %4)")
            .arg(m_backgroundColor.red())
            .arg(m_backgroundColor.green())
            .arg(m_backgroundColor.blue())
            .arg(m_backgroundColor.alpha() / 255.0);
    emit EAiExec()->sigMainContentBackgroundColor(backgroundColor);

    titlebar()->setPalette(pa);

    update();
}

void ChatWindow::showToast(const QString &message)
{
    DFloatingMessage *floatMessage = new DFloatingMessage(DFloatingMessage::TransientType, this);
    floatMessage->setMessage(message);

    QRect geometry(QPoint(10, 10), floatMessage->sizeHint());
    geometry.moveCenter(rect().center());

    //Show the message at 1/3 from window's bottom
    geometry.moveBottom(rect().bottom() - (rect().height() / 3));
    floatMessage->setGeometry(geometry);
    floatMessage->show();
}

void ChatWindow::onSystemThemeChanged()
{
    qCInfo(logAIGUI) << "System theme changed";
    updateSystemTheme();
}

void ChatWindow::onScreenOrDockChanged()
{
    //keep the width
    if(m_displayMode == SIDEBAR_MODE)
        updateSidebarGeometry(this->width());
}

void ChatWindow::showWindow(ChatIndex index)
{
    // 写作智能体禁用数字形象
    bool shouldDisableDigitalImage = m_isDigitalImageDisable || m_voiceConversationDisabled || EAiExec()->currentAssistantType() == AssistantType::AI_WRITING;
    if (shouldDisableDigitalImage && index == ChatIndex::Talk) {
        m_isDigitalImageDisable = true;
        m_isActiveChatFromDigitalImage = true;
        emit EAiExec()->sigActiveChatFromDigitalImage();
        index = ChatIndex::Text;
        qCInfo(logAIGUI) << "Talk to chat";
    }

    if (ChatIndex::Talk == index) {
        //Requirement: if there is a modal window, do not do the switch and display the talk window
        if (QApplication::activeModalWidget()) {
            qCWarning(logAIGUI) << "Cannot show talk window while modal dialog is active";
            return;
        }
        if (m_voiceConversationDisabled)
            return;

        m_webView->setToDigitalMode();
        m_isDigitalMode = true;
        m_chatBtn->updateActiveStatus(m_isDigitalMode);
        if (m_displayMode == SIDEBAR_MODE)
            setWindowIconVisible(false);
        setWindowTitleVisible(false);
    }

    if (isHidden() || isMinimized()) {
        if (m_displayMode == SIDEBAR_MODE) {
            showSidebarMode();
            return;
        } else {
            showWindowMode();

            m_stackedWidget->show();
            m_stackedWidget->setCurrentIndex(0);
            return;
        }
    } else if (!this->isActiveWindow()) { // 由于出现了划词、写作，改写(qApp->applicationState() != Qt::ApplicationActive)
        onlyShowWidget();
        return;
    }

    if (ChatIndex::Talk != index) {
        if (m_displayMode == SIDEBAR_MODE) {
            closeWindow();
        } else {
#ifdef COMPILE_ON_V25
            hide();
#else
            showMinimized();
#endif
        }
    }

}

void ChatWindow::closeWindow()
{
    if (!isHidden()) {
        qCInfo(logAIGUI) << "Closing window";
        if (m_displayMode == SIDEBAR_MODE) {
            updateSidebarGeometry(this->width());
            hide();
        } else {
            this->hide();
        }
    }
}

void ChatWindow::onlyShowWindow()
{
    // tid:1001600006 event:chatwindow
    ReportIns()->writeEvent(report::ChatwindowPoint().assemblingData());

    if (isHidden()) {
        if (m_displayMode == SIDEBAR_MODE) {
            showSidebarMode();
            return;
        } else {
            showWindowMode();

            m_stackedWidget->show();
            m_stackedWidget->setCurrentIndex(0);
            return;
        }
    } else if (!this->isActiveWindow()) {
        onlyShowWidget();
        return;
    }
}

void ChatWindow::onlyShowWidget()
{
    if (isHidden()) {
        //Resume on redisplay
        qCDebug(logAIGUI) << "Updating sidebar geometry for hidden window";
        updateSidebarGeometry();
    }

    activeShowWindow();
}

void ChatWindow::showAboutWindow()
{
    qCInfo(logAIGUI) << "Showing about window";

    auto sidebarAboutDialogPos=[](const QRect &windowRect, const QRect &dialogRect){
        QPoint newAboutDialogPos;
        if (dialogRect.width() > windowRect.width()) {
            newAboutDialogPos.setX(windowRect.x() + windowRect.width() - dialogRect.width());
            newAboutDialogPos.setY((windowRect.height() - windowRect.y() - dialogRect.height()) / 2);
        }

        return newAboutDialogPos;
    };

    auto aboutDialog = this->findChild<DAboutDialog *>();
    if (aboutDialog) {
        if (DisplayMode::WINDOW_MODE == m_displayMode) {
            aboutDialog->moveToCenter();
        } else if (DisplayMode::SIDEBAR_MODE == m_displayMode) {
            QPoint pos = sidebarAboutDialogPos(this->geometry(), aboutDialog->geometry());
            aboutDialog->move(pos);
        }

        aboutDialog->show();
        aboutDialog->activateWindow();
        return;
    }

    aboutDialog = new DAboutDialog(this);
    aboutDialog->setProductName("UOS AI");
    aboutDialog->setProductIcon(QIcon::fromTheme(appIcon));
    aboutDialog->setVersion(QApplication::applicationVersion());
    aboutDialog->setDescription(tr("UOS AI is a desktop smart assistant, your personal assistant! You can communicate with it using text or voice, and it can help answer questions, provide information, and generate images based on your descriptions."));

    if (DisplayMode::SIDEBAR_MODE == m_displayMode) {
        QPoint pos = sidebarAboutDialogPos(this->geometry(), aboutDialog->geometry());
        aboutDialog->move(pos);
    }

    // 备案信息
    QLabel *recordTitleLabel = nullptr;
    QLabel *recordNumLabel   = nullptr;
    recordNumLabel = aboutDialog->findChild<QLabel *>("LicenseLabel");
    if (recordNumLabel) {
        qCDebug(logAIGUI) << "licenseLabel FOUND";
        QList<QLabel *> list = aboutDialog->findChildren<QLabel *>();
        for (int i = 0; i < list.size(); i++) {
            if (list[i] == recordNumLabel && i > 0) {
                qCDebug(logAIGUI) << "licenseTipLabel FOUND";
                recordTitleLabel = list[i - 1];
                break;
            }
        }
    }
    if (recordTitleLabel && recordNumLabel) {
        // <a href='%1' style='text-decoration: none; color: rgba(0,129,255,0.9);'>%2</a>
        recordTitleLabel->setText(tr("Filing Information"));
        aboutDialog->setLicense("BeiJing-UOSAIZhiNengZhuShou-20250312S0018");
        aboutDialog->setFixedHeight(aboutDialog->height() + 50);
    }

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, &ChatWindow::onAboutFontChanged);

    aboutDialog->show();
    this->onAboutFontChanged();
}

void ChatWindow::onAboutFontChanged()
{
    static const QString TEXT("BeiJing-UOSAIZhiNengZhuShou-20250312S0018");

    auto aboutDialog = this->findChild<DAboutDialog *>();
    if (aboutDialog == nullptr) {
        return;
    }

    QLabel *recordNumLabel = aboutDialog->findChild<QLabel *>("LicenseLabel");
    if (recordNumLabel == nullptr) {
        return;
    }

    QString tempText = TEXT;
    QFontMetrics fm(recordNumLabel->font());
    while (fm.horizontalAdvance(tempText) > recordNumLabel->width()) {
        tempText.remove(tempText.length() - 1, 1);
    }
    if (tempText != TEXT) {
        recordNumLabel->setText(tempText + "\n" + TEXT.right(TEXT.length() - tempText.length()));
    } else {
        recordNumLabel->setText(tempText);
    }
}

void ChatWindow::setMenuDisabled(bool disabled)
{
    qCInfo(logAIGUI) << "Setting menu disabled:" << disabled;
    titlebar()->setMenuDisabled(disabled);
}
void ChatWindow::setVoiceConversationDisabled(bool disabled)
{
    qCInfo(logAIGUI) << "Setting voice conversation disabled:" << disabled;

    m_voiceConversationDisabled = disabled;
    
    // 综合考虑所有状态因素来设置按钮的禁用状态
    bool shouldDisable = m_isDigitalImageDisable || m_voiceConversationDisabled;
    
    // 检查语音对话状态
    if (m_voiceConversationStatus == 3 || m_voiceConversationStatus == 4) {
        shouldDisable = true;
    }
    
    m_chatBtn->setDisabled(shouldDisable);
}

void ChatWindow::setChatButtonVisible(bool visible)
{
    qCInfo(logAIGUI) << "Setting chat button visible:" << visible;
    m_chatBtn->setVisible(visible);
}

void ChatWindow::setWindowTitleVisible(bool visible)
{
    qCInfo(logAIGUI) << "Setting window title visible:" << visible;
    if (visible)
        titlebar()->setTitle(EAiExec()->currentAssistantName());
    else
        titlebar()->setTitle("");
}

void ChatWindow::setWindowIconVisible(bool visible)
{
    qCInfo(logAIGUI) << "Setting window icon visible:" << visible;
    DIconButton *btn = titlebar()->findChild<DIconButton *>();
    // 在窗口不显示的情况下调用titlebar()->setIcon，其内部会无限插入spacing，dtk bug。
    // 通过查阅 DTitlebar 源码，直接操作图标控件.
    // 可能存在兼容问题。
    if (btn && btn->accessibleName() == "DTitlebarIconLabel") {
        qCDebug(logAIGUI) << "found DTitlebar icon btn.";
        if (visible)
            btn->setIcon(QIcon::fromTheme(kApplicationIconName));
        else
            btn->setIcon(QIcon());
        return;
    }

    if (visible)
        titlebar()->setIcon(QIcon::fromTheme(kApplicationIconName));
    else
        titlebar()->setIcon(QIcon());
}

void ChatWindow::setHasChatHistory(bool hasChatHistory)
{
    qCInfo(logAIGUI) << "Setting chat history status:" << hasChatHistory;
    m_hasChatHistory = hasChatHistory;
    if(m_hasChatHistory) {
        if(!m_isDigitalMode)
            setWindowTitleVisible(true);

        if (m_displayMode == SIDEBAR_MODE)
            setWindowIconVisible(true);
    } else {
        setWindowTitleVisible(false);
        if (m_displayMode == SIDEBAR_MODE)
            setWindowIconVisible(false);
    }
}

bool ChatWindow::isDigitalMode()
{
    return m_isDigitalMode;
}

void ChatWindow::setRedPointSource(RedPointSource source, bool visible)
{
    if (visible) {
        // 设置对应位，启用该来源的红点
        m_redPointSources |= source;
    } else {
        // 清除对应位，禁用该来源的红点
        m_redPointSources &= ~source;
    }

    // 更新 OptionButton 的红点状态
    updateOptionButtonRedPoint();
}

void ChatWindow::updateOptionButtonRedPoint()
{
    auto optionButton = titlebar()->findChild<DIconButton *>("DTitlebarDWindowOptionButton");

    if (optionButton) {
        // 只要有一个来源启用红点，就显示红点
        bool hasRedPoint = (m_redPointSources != 0);
        DStyle::setRedPointVisible(optionButton, hasRedPoint);
        optionButton->update();
        qCDebug(logAIGUI) << "OptionButton red point updated:" << hasRedPoint << "(sources:" << m_redPointSources << ")";
    }
}
