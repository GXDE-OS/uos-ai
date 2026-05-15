#include "sidewindow.h"

#include "gui/web/webcontext.h"

#include <QApplication>
#include <QWindow>
#include <QLoggingCategory>
#include <QThread>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

#ifdef COMPILE_ON_QT6
#define CHANGESPLITWINDOW_VAR "_d_splitWindowOnScreenByType"
#define GETSUPPORTSPLITWINDOW_VAR "_d_supportForSplittingWindowByType"
#else
#define CHANGESPLITWINDOW_VAR "_d_splitWindowOnScreen"
#define GETSUPPORTSPLITWINDOW_VAR "_d_supportForSplittingWindow"
#endif

SideWindow::SideWindow(QWidget *parent) : AppWindow(parent)
{

}

void SideWindow::initWindow(uos_ai::WebContext *ctx)
{
    AppWindow::initWindow(ctx);
    createWinId();

    windowHandle()->installEventFilter(this);
}

WindowMode SideWindow::mode() const
{
    return WindowMode::WmSide;
}

bool SideWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == windowHandle()) {
        if (event->type() == QEvent::Expose) {
            obj->removeEventFilter(this);
            qCDebug(logAIGUI) << "the side window is activated";
            QMetaObject::invokeMethod(this, "splitWindow", Qt::QueuedConnection);
            return false;
        }
    }
    return false;
}
#ifdef COMPILE_ON_QT6
void SideWindow::splitWindow()
{
    // copy from DSplitScreen splitWindowOnScreenByType
    /*
        enum PositionFlag {
        Left = 1 << 0,
        Right = 1 << 1,
        Top = 1 << 2,
        Bottom = 1 << 3,
    };

    enum ModeFlag {
        TwoSplit = 1,
        ThreeSplit = 2,
        FourSplit = 4,
        SupportTwoSplit = TwoSplit,
        SupportFourSplit = ThreeSplit,
        PositionType = 1 << 4,
        Left = 1 << (PositionType + 1)
    };

     */
    QFunctionPointer splitWindowOnScreen = qApp->platformFunction(CHANGESPLITWINDOW_VAR);

    if (!splitWindowOnScreen) {
        qCWarning(logAIGUI) << "Can't get handler for `splitWindowOnScreenByType` of platform function";
        return;
    }

    const QWindow *windowHandle = this->windowHandle();
    if (windowHandle) {
        int maxTry = 100;
        while (!windowHandle->isExposed() && maxTry--) {
            QThread::msleep(5);
        }
        qCDebug(logAIGUI) << "Call `splitWindowOnScreenByType` of platform function," << maxTry;
        reinterpret_cast<void(*)(quint32, quint32, quint32)>(splitWindowOnScreen)(windowHandle->winId(), 1 << 1, 1); // Right TwoSplit
    } else {
        qCWarning(logAIGUI) << "Can't get window handle for side window";
    }
}
#else
void SideWindow::splitWindow()
{
    // copy from DTitlebarPrivate::changeWindowSplitedState
    /*
    enum SplitScreenMode {
        SplitLeftHalf = 1,
        SplitRightHalf = 2,
        SplitFullScreen = 15
    };
    */
    QFunctionPointer splitWindowOnScreen = qApp->platformFunction(CHANGESPLITWINDOW_VAR);
    if (!splitWindowOnScreen) {
        qCWarning(logAIGUI) << "Can't get handler for `splitWindowOnScreen` of platform function";
        return;
    }

    const QWindow *windowHandle = this->windowHandle();
    if (windowHandle) {
        int maxTry = 100;
        while (!windowHandle->isExposed() && maxTry--) {
            QThread::msleep(5);
        }
        qCDebug(logAIGUI) << "Call `splitWindowOnScreen` of platform function," << maxTry;
        reinterpret_cast<void(*)(quint32, quint32)>(splitWindowOnScreen)(windowHandle->winId(), 2); //SplitRightHalf
    } else {
        qCWarning(logAIGUI) << "Can't get window handle for side window";
    }
}
#endif
