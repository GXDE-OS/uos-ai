#include "windowchannel.h"
#include "gui/window/windowmanager.h"
#include "app/application.h"
#include "database/appdatabase.h"

#include <DWindowManagerHelper>
#include <DGuiApplicationHelper>

#include <QJsonObject>
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QWebEngineView>
#include <QStyleHints>
#include <QLoggingCategory>
#include <QEvent>

#ifndef COMPILE_ON_QT6
#include <qpa/qplatformwindow.h>
#endif

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DGUI_USE_NAMESPACE;
using namespace uos_ai;

inline constexpr int kDefaultMainWindowSidebarWidth = 200;

WindowChannel::WindowChannel(QObject *parent)
    : QObject(parent)
{

}

void WindowChannel::minimize()
{
    if (window)
        window->showMinimized();
}

void WindowChannel::maximize()
{
    if (window)
        window->showMaximized();
}

void WindowChannel::restore()
{
    if (window)
        window->showNormal();
}

void WindowChannel::close()
{
    if (window)
        window->close();
}

void WindowChannel::startMove(int sx, int sy, int tx, int ty)
{
    if (QPoint(tx - sx, ty -sy).manhattanLength() < QGuiApplication::styleHints()->startDragDistance()) {
        return;
    }

    if (webView && window) {
        auto render = webView->findChild<QWidget *>();
        if (render) {
            QMouseEvent e(QEvent::MouseButtonRelease, QPoint(sx, sy), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(render, &e);
#ifndef COMPILE_ON_QT6
            window->windowHandle()->handle()->startSystemMove(QPoint(sx, sy));
#else
            window->windowHandle()->startSystemMove();
#endif
        } else {
            qCWarning(logAIGUI) << "can not find web render.";
        }
    } else {
        qCWarning(logAIGUI) << "no web view to move.";
    }
}

void WindowChannel::systemMenu()
{
    if (window)
        DWindowManagerHelper::popupSystemWindowMenu(window->windowHandle());
}

void WindowChannel::switchMode(int mode)
{
    WmIns()->showWindow(static_cast<WindowMode>(mode));
}

int WindowChannel::windowMode()
{
    return WmIns()->currentMode();
}

void WindowChannel::ensureMinimumWidth(int width)
{
    if (!window || width <= 0 || window->isMaximized()) {
        return;
    }

    if (window->width() >= width) {
        return;
    }

    // 仅在需要时补足窗口宽度，保持当前高度不变，
    // 供前端在展开侧边栏前先为“侧边栏最小宽度 + 工作区最小宽度”预留空间。
    window->resize(width, window->height());
}

void WindowChannel::saveMainWindowSidebarState(int width, bool expanded)
{
    const int sidebarWidth = width > 0 ? width : kDefaultMainWindowSidebarWidth;
    AppDatabase::instance()->saveConfigInt(CONFIG_MAIN_WINDOW_SIDEBAR_WIDTH, sidebarWidth);
    AppDatabase::instance()->saveConfigBool(CONFIG_MAIN_WINDOW_SIDEBAR_EXPANDED, expanded);
}

QJsonObject WindowChannel::getMainWindowSidebarState()
{
    return QJsonObject {
        {"sidebarWidth", persistedMainWindowSidebarWidth()},
        {"sidebarExpanded", persistedMainWindowSidebarExpanded()},
    };
}

void WindowChannel::showAboutWindow()
{
    WmIns()->showAboutWindow();
}

void WindowChannel::showHelpWindow()
{
    DGuiApplicationHelper::instance()->handleHelpAction();
}

void WindowChannel::showConfig(int page)
{
    aiApp->showConfig(page);
}

bool WindowChannel::isMainWindowActive()
{
    if (!WmIns()->isWindowExist()) {
        return false;
    }
    return WmIns()->isActiveWindow();
}

int WindowChannel::persistedMainWindowSidebarWidth()
{
    const int width = AppDatabase::instance()->getConfigInt(CONFIG_MAIN_WINDOW_SIDEBAR_WIDTH);
    return width > 0 ? width : kDefaultMainWindowSidebarWidth;
}

bool WindowChannel::persistedMainWindowSidebarExpanded()
{
    const QString expanded = AppDatabase::instance()->getConfig(CONFIG_MAIN_WINDOW_SIDEBAR_EXPANDED);
    if (expanded.isEmpty()) {
        return true;
    }

    return expanded.toInt() != 0;
}

bool WindowChannel::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == window && event->type() == QEvent::ApplicationFontChange) {
        qCDebug(logAIGUI) << "ApplicationFontChange event received";
        // Handle font change event here
        // For example, notify web view or update UI
        QFontInfo info(window->font());
        QString fontInfoStr = info.family() + "#" + QString::number(info.pixelSize());
        QMetaObject::invokeMethod(this, "windowFontChanged", Qt::QueuedConnection, Q_ARG(QString, fontInfoStr));
    }

    return QObject::eventFilter(watched, event);
}

void WindowChannel::showToast(const QString &type, const QString &message)
{
    emit toastRequested(type, message);
}
