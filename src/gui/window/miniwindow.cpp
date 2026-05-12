#include "miniwindow.h"

#include "ddedockobject.h"
#include <QLoggingCategory>
#include <QApplication>
#include <QScreen>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)
using namespace  uos_ai;

MiniWindow::MiniWindow(QWidget *parent) : AppWindow(parent)
{
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
}

void MiniWindow::initWindow(WebContext *ctx)
{
    AppWindow::initWindow(ctx);

    dockApi = new DDeDockObject(this);
    connect(dockApi, &DDeDockObject::FrontendWindowRectChanged, this, &MiniWindow::updateGeometry, Qt::QueuedConnection);

    updateGeometry();
}

WindowMode MiniWindow::mode() const
{
    return WindowMode::WmMini;
}

void MiniWindow::updateGeometry()
{
    // 获取dock相关配置
    int displayMode = dockApi->displayMode();
    DockPosition dockPosition = static_cast<DockPosition>(dockApi->position());
    qCInfo(logAIGUI) << "dockPosition: " << dockPosition;

    // 获取屏幕和设备信息
    qreal devicePixelRatio = qApp->primaryScreen()->devicePixelRatio();
    QRect dockRect = dockApi->frontendWindowRect();
    qCInfo(logAIGUI) << "frontendWindowRect: " << dockRect;

    // 确定当前显示屏幕
    QRect displayRect = findDisplayScreenContaining(dockRect, devicePixelRatio);
    qCInfo(logAIGUI) << "Current displayRect: " << displayRect;

    // 计算窗口尺寸和位置
    const int defaultMargin = 10;
    const int windowWidth = 350;
    const int windowHeight = 250;
    
    int maxWindowHeight = calculateMaxHeight(displayRect, dockRect, dockPosition, 
                                           displayMode, devicePixelRatio, defaultMargin);
    
    QPoint windowPosition = calculateWindowPosition(displayRect, dockRect, dockPosition,
                                                   displayMode, devicePixelRatio, 
                                                   windowWidth, defaultMargin);
    
    // 设置窗口几何属性
    QRect geometry(windowPosition.x(), 
                   windowPosition.y() + maxWindowHeight - windowHeight,
                   windowWidth, windowHeight);
    
    qCInfo(logAIGUI) << "mini window geometry: " << geometry;
    
    setMaximumHeight(maxWindowHeight);
    setMinimumHeight(windowHeight);
    setFixedWidth(windowWidth);
    setGeometry(geometry);
}

// 辅助函数：查找包含dock中心的屏幕
QRect MiniWindow::findDisplayScreenContaining(const QRect &dockRect, qreal devicePixelRatio) const
{
    QList<QScreen*> screens = QGuiApplication::screens();
    
    for (const QScreen *screen : screens) {
        QRect screenRect(screen->geometry().x(), screen->geometry().y(),
                        screen->geometry().width() * devicePixelRatio,
                        screen->geometry().height() * devicePixelRatio);
        qCInfo(logAIGUI) << "screenRect: " << screenRect;
        
        if (screenRect.contains(dockRect.center())) {
            return screen->geometry();
        }
    }
    
    // 如果没有找到匹配的屏幕，使用主屏幕
    return QGuiApplication::primaryScreen()->geometry();
}

// 辅助函数：计算最大窗口高度
int MiniWindow::calculateMaxHeight(const QRect &displayRect, const QRect &dockRect,
                                  DockPosition dockPosition, int displayMode,
                                  qreal devicePixelRatio, int margin) const
{
    int baseHeight = displayRect.height() - margin * 2;
    
    // 只有在dock位于顶部或底部时才需要调整高度
    if (dockPosition == DockPosition::Top || dockPosition == DockPosition::Bottom) {
        int dockHeightAdjustment = dockRect.height() / devicePixelRatio;
        
        if (displayMode == DockModel::Fashion) {
            return baseHeight - dockHeightAdjustment - 0; // dockMargin为0
        } else {
            return baseHeight - dockHeightAdjustment;
        }
    }
    
    return baseHeight;
}

// 辅助函数：计算窗口位置
QPoint MiniWindow::calculateWindowPosition(const QRect &displayRect, const QRect &dockRect,
                                          DockPosition dockPosition, int displayMode,
                                          qreal devicePixelRatio, int windowWidth, 
                                          int margin) const
{
    int x = displayRect.x() + displayRect.width() - windowWidth - margin;
    int y = displayRect.y() + margin;
    
    // 调整X坐标（dock在右侧时）
    if (dockPosition == DockPosition::Right) {
        int dockWidthAdjustment = dockRect.width() / devicePixelRatio;
        
        if (displayMode == DockModel::Fashion) {
            x = displayRect.x() + displayRect.width() - (windowWidth + dockWidthAdjustment + 0 + margin);
        } else {
            x = displayRect.x() + displayRect.width() - (windowWidth + dockWidthAdjustment + margin);
        }
    }
    
    // 调整Y坐标（dock在顶部时）
    if (dockPosition == DockPosition::Top) {
        int dockHeightAdjustment = dockRect.height() / devicePixelRatio;
        
        if (displayMode == DockModel::Fashion) {
            y = displayRect.y() + margin + dockHeightAdjustment + 0; // dockMargin为0
        } else {
            y = displayRect.y() + margin + dockHeightAdjustment;
        }
    }
    
    return QPoint(x, y);
}