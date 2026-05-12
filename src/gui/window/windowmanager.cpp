#include "windowmanager.h"
#include "mainwindow.h"
#include "miniwindow.h"
#include "sidewindow.h"
#include "aboutwindow.h"
#include "gui/web/webcontext.h"
#include "gui/web/appwebpage.h"
#include "gui/web/filechannel.h"
#include "gui/web/audiochannel.h"
#include "gui/web/taskchannel.h"
#include "gui/web/reportchannel.h"
#include "services/screenshot/screenshotservice.h"
#include "services/fileservice/fileservice.h"
#include "database/appdatabase.h"

#include <QWebChannel>
#include <QThread>
#include <QDebug>
#include <QApplication>
#include <QLoggingCategory>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

WindowManager::WindowManager(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
}

WindowManager::~WindowManager()
{
    if (curWindow) {
        curWindow->takeCentralWidget();
        delete curWindow;
        curWindow = nullptr;
    }

    if (ctx) {
        ctx->reset();
        delete ctx;
        ctx = nullptr;
    }
}

WindowManager *WindowManager::instance()
{
    static WindowManager wm;
    return &wm;
}

void WindowManager::createContext()
{
    if (ctx)
        return;

    ctx = new WebContext;
    ctx->appView = new AppWebView;

    auto page = new AppWebPage(ctx->appView);
    page->setWebChannel(new QWebChannel(page));
    ctx->appView->setPage(page);

    ctx->winCh = new WindowChannel;
    page->webChannel()->registerObject("windowObj", ctx->winCh);
    ctx->winCh->setWebView(ctx->appView);

    ctx->sessCh = new SessionChannel;
    page->webChannel()->registerObject("sessObj", ctx->sessCh);

    ctx->assistantCh = new AssistantChannel;
    page->webChannel()->registerObject("assistObj", ctx->assistantCh);

    ctx->systemCh = new SystemChannel;
    page->webChannel()->registerObject("systemObj", ctx->systemCh);

    ctx->serviceConfigCh = new ServiceConfigChannel;
    page->webChannel()->registerObject("serviceConfigObj", ctx->serviceConfigCh);

    ctx->conversationCh = new ConversationChannel;
    page->webChannel()->registerObject("conversationObj", ctx->conversationCh);

    ctx->fileCh = new FileChannel;
    page->webChannel()->registerObject("fileObj", ctx->fileCh);
    connect(ctx->appView, &AppWebView::nativeFilesDropped, ctx->fileCh, &FileChannel::emitNativeDrop);
    
    ctx->audioCh = new AudioChannel;
    page->webChannel()->registerObject("audioObj", ctx->audioCh);
    
    ctx->taskCh = new TaskChannel;
    page->webChannel()->registerObject("taskObj", ctx->taskCh);
    ctx->taskCh->setWebContext(ctx);

    ctx->skillsMgr = new SkillsManager;
    page->webChannel()->registerObject("skillsMgr", ctx->skillsMgr);
    ctx->skillsMgr->setupFileWatcher();

    ctx->reportCh = new ReportChannel;
    page->webChannel()->registerObject("reportObj", ctx->reportCh);

    // Connect screenshot requested signal from FileChannel
    connect(ctx->fileCh, &FileChannel::screenshotRequested, this, &WindowManager::handleScreenshotRequested);

    // Connect toast requested signal from WindowChannel
    connect(ctx->fileCh, &FileChannel::toastRequested, this, &WindowManager::handleToastRequested);

    // Connect screenshot service done/canceled signals
    connect(ScreenshotService::instance(), &ScreenshotService::done, this, &WindowManager::handleScreenshotDone);
    connect(ScreenshotService::instance(), &ScreenshotService::canceled, this, &WindowManager::handleScreenshotCanceled);

    ctx->appView->setUrl(QUrl("qrc:/web/dist/index.html"));
}

void WindowManager::showWindow(WindowMode mode)
{
    createContext();
    // 目前写死，只有主窗口
    mode = WmMain;

    if (curWindow) {
        if (curWindow->mode() == mode) {
            if (curWindow->isHidden()) {
                curWindow->show();
            } else if(!curWindow->isActiveWindow() || curWindow->isMinimized()){
                curWindow->activateWindow();
            }
            return;
        }

        curWindow->hide();
        curWindow->deleteLater();
        curWindow = nullptr;
    }
    
    // Save new window mode to database
    AppDatabase::instance()->saveConfigInt(CONFIG_WINDOW_MODE, mode);
    
    switch (mode) {
    case WindowMode::WmMain: {
        curWindow = new MainWindow;
        curWindow->initWindow(ctx);
        curWindow->show();
        break;
    }
    case WindowMode::WmSide: {
        curWindow = new SideWindow;
        curWindow->initWindow(ctx);
        curWindow->setMinimumWidth(200);
        curWindow->setMaximumWidth(500);
        curWindow->show();
        break;
    }
    case WindowMode::WmMini: {
        curWindow = new MiniWindow;
        curWindow->initWindow(ctx);
        curWindow->show();
        break;
    }
    default:
        qCCritical(logAIGUI) << "invaild window mode:" << mode;
    }

    emit ctx->winCh->windowModeChanged(mode);
}

void WindowManager::hideWindow()
{
#ifdef COMPILE_ON_V25
    curWindow->hide();
#else
    curWindow->showMinimized();
#endif
}

WindowMode WindowManager::currentMode() const
{
    if (curWindow)
        return curWindow->mode();

    //first load saved window mode from database, default to WmMain if not found
    WindowMode startupMode = static_cast<WindowMode>(AppDatabase::instance()->getConfigInt(CONFIG_WINDOW_MODE));
    if (startupMode < WindowMode::WmNone || startupMode > WmSide) {
        startupMode = WindowMode::WmMain;
    }

    return startupMode;
}

bool WindowManager::isWindowExist()
{
    return curWindow;
}

bool WindowManager::isWindowVisible()
{
    return curWindow && curWindow->isVisible() && !curWindow->isMinimized();
}

bool WindowManager::isActiveWindow()
{
    return curWindow && curWindow->isActiveWindow();
}

void WindowManager::showAboutWindow()
{
    if (aboutWindow) {
        aboutWindow->showDialog();
        return;
    }

    aboutWindow = new AboutWindow;
    aboutWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(aboutWindow, &QObject::destroyed, this, [this](){
        aboutWindow = nullptr;
    });

    aboutWindow->showDialog();
}

WebContext* WindowManager::context() const
{
    return ctx;
}

void WindowManager::handleScreenshotRequested()
{
    // 先隐藏聊天窗口
    if (isWindowExist() && isWindowVisible()) {
        hideWindow();
        // 使用QTimer确保窗口隐藏完成后再开始截图
        QTimer::singleShot(500, this, []() {
            ScreenshotService::instance()->start();
        });
    } else {
        // 如果窗口已经隐藏或不存在，直接开始截图
        ScreenshotService::instance()->start();
    }
}

void WindowManager::handleScreenshotDone(const QString &imagePath)
{
    // 恢复聊天窗口显示
    showWindow(currentMode());

    // Process the screenshot file through FileChannel
    if (ctx && ctx->fileCh) {
        ctx->fileCh->emitIncomingFiles(
            QStringList{ imagePath },
            QString(),
            -1,
            QStringLiteral("handleScreenshotFile")
        );
    }
}

void WindowManager::handleScreenshotCanceled()
{
    // 恢复聊天窗口显示
    showWindow(currentMode());
}

void WindowManager::handleToastRequested(const QString &type, const QString &message)
{
    // 激活窗口以确保用户能看到 toast 提示
    if (!isWindowExist() || !isActiveWindow()) {
        showWindow(currentMode());
    }
    ctx->winCh->showToast(type, message);
}
