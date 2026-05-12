#include "mainwindow.h"
#include "gui/web/webcontext.h"
#include "database/appdatabase.h"

#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QResizeEvent>
#include <QTimer>
#include <QWindow>

using namespace uos_ai;

inline constexpr int kDefaultMainWindowWidth = 1022;
inline constexpr int kDefaultMainWindowHeight = 700;

MainWindow::MainWindow(QWidget *parent) : AppWindow(parent)
{
    m_persistSizeTimer = new QTimer(this);
    m_persistSizeTimer->setSingleShot(true);
    connect(m_persistSizeTimer, &QTimer::timeout, this, &MainWindow::flushPendingWindowSize);
}

void MainWindow::initWindow(WebContext *ctx)
{
    AppWindow::initWindow(ctx);

    setMinimumWidth(560);
    setMinimumHeight(560);
    restorePersistedSize();

    createWinId();
    auto window = windowHandle();
    connect(window, &QWindow::windowStateChanged, ctx->winCh, &WindowChannel::windowStateChanged);
}

WindowMode MainWindow::mode() const
{
    return WindowMode::WmMain;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (windowState() != Qt::WindowNoState) {
        return;
    }

    m_pendingWindowSize = event->size();
    m_persistSizeTimer->start(1000);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    flushPendingWindowSize();
    QMainWindow::closeEvent(event);
}

void MainWindow::restorePersistedSize()
{
    const QSize persistedSize = persistedMainWindowSize();
    const int width = persistedSize.width();
    const int height = persistedSize.height();

    resize(qMax(width, minimumWidth()), qMax(height, minimumHeight()));
}

void MainWindow::flushPendingWindowSize()
{
    if (m_persistSizeTimer) {
        m_persistSizeTimer->stop();
    }

    if (m_pendingWindowSize.isEmpty() || windowState() != Qt::WindowNoState) {
        return;
    }

    saveMainWindowSize(m_pendingWindowSize);
    m_pendingWindowSize = QSize();
}


QSize MainWindow::persistedMainWindowSize()
{
    const QString rawSize = AppDatabase::instance()->getConfig(CONFIG_MAIN_WINDOW_SIZE);
    if (rawSize.isEmpty()) {
        return QSize(kDefaultMainWindowWidth, kDefaultMainWindowHeight);
    }

    QJsonParseError parseError;
    const QJsonDocument sizeDoc = QJsonDocument::fromJson(rawSize.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !sizeDoc.isObject()) {
        return QSize(kDefaultMainWindowWidth, kDefaultMainWindowHeight);
    }

    const QJsonObject sizeObj = sizeDoc.object();
    const int width = sizeObj.value("width").toInt(kDefaultMainWindowWidth);
    const int height = sizeObj.value("height").toInt(kDefaultMainWindowHeight);

    return QSize(width > 0 ? width : kDefaultMainWindowWidth,
                 height > 0 ? height : kDefaultMainWindowHeight);
}

void MainWindow::saveMainWindowSize(const QSize &size)
{
    if (!size.isValid() || size.width() <= 0 || size.height() <= 0) {
        return;
    }

    const QJsonObject sizeObj {
                              {"width", size.width()},
                              {"height", size.height()},
                              };

    AppDatabase::instance()->saveConfig(CONFIG_MAIN_WINDOW_SIZE,
                                        QString::fromUtf8(QJsonDocument(sizeObj).toJson(QJsonDocument::Compact)));
}
