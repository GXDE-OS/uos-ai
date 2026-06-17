#include "windowchannel.h"
#include "gui/window/windowmanager.h"
#include "app/application.h"
#include "database/appdatabase.h"

#include <DWindowManagerHelper>
#include <DGuiApplicationHelper>

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QCoreApplication>
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

// 仅在需要显示新的引导页时递增，数据库只保存这个枚举值。
enum NewUserGuideVersion {
    Guide_3_0 = 1,
};

inline constexpr int kCurrentNewUserGuideVersion = Guide_3_0;

static QJsonObject normalizedSidebarGroupCollapsedStates(const QJsonObject &rawStates)
{
    QJsonObject normalizedStates;
    for (auto it = rawStates.constBegin(); it != rawStates.constEnd(); ++it) {
        if (it.value().isBool()) {
            normalizedStates.insert(it.key(), it.value().toBool());
        }
    }
    return normalizedStates;
}

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

void WindowChannel::saveMainWindowSidebarGroupCollapsedStates(const QString &groupCollapsedStatesJson)
{
    QJsonObject groupCollapsedStates;

    if (!groupCollapsedStatesJson.isEmpty()) {
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(groupCollapsedStatesJson.toUtf8(), &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            groupCollapsedStates = normalizedSidebarGroupCollapsedStates(doc.object());
        } else {
            qCWarning(logAIGUI) << "Failed to parse main window sidebar group collapsed states for persistence:"
                                << error.errorString();
        }
    }

    const QString json = QString::fromUtf8(QJsonDocument(groupCollapsedStates).toJson(QJsonDocument::Compact));
    AppDatabase::instance()->saveConfig(CONFIG_MAIN_WINDOW_SIDEBAR_GROUP_COLLAPSED_STATES, json);
}

QJsonObject WindowChannel::getMainWindowSidebarState()
{
    return QJsonObject {
        {"sidebarWidth", persistedMainWindowSidebarWidth()},
        {"sidebarExpanded", persistedMainWindowSidebarExpanded()},
        {"groupCollapsedStates", persistedMainWindowSidebarGroupCollapsedStates()},
    };
}

bool WindowChannel::shouldShowNewUserGuideOnStartup()
{
    const QString shownVersionText = AppDatabase::instance()
            ->getConfig(CONFIG_NEW_USER_GUIDE_SHOWN_VERSION)
            .trimmed();

    if (shownVersionText.isEmpty()) {
        return true;
    }

    bool ok = false;
    const int shownVersion = shownVersionText.toInt(&ok);
    if (!ok) {
        AppDatabase::instance()->saveConfigInt(
                CONFIG_NEW_USER_GUIDE_SHOWN_VERSION,
                kCurrentNewUserGuideVersion);
        return false;
    }

    return shownVersion < kCurrentNewUserGuideVersion;
}

void WindowChannel::recordNewUserGuideShown()
{
    AppDatabase::instance()->saveConfigInt(
            CONFIG_NEW_USER_GUIDE_SHOWN_VERSION,
            kCurrentNewUserGuideVersion);
}

void WindowChannel::showAboutWindow()
{
    WmIns()->showAboutWindow();
}

void WindowChannel::showUpdateLogWindow()
{
    WmIns()->showUpdateLogWindow();
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

QJsonObject WindowChannel::persistedMainWindowSidebarGroupCollapsedStates()
{
    const QString json = AppDatabase::instance()->getConfig(CONFIG_MAIN_WINDOW_SIDEBAR_GROUP_COLLAPSED_STATES);
    if (json.isEmpty()) {
        return {};
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qCWarning(logAIGUI) << "Failed to parse persisted main window sidebar group collapsed states:"
                             << error.errorString();
        return {};
    }

    return normalizedSidebarGroupCollapsedStates(doc.object());
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
