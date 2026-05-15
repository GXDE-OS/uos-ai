#include "appwindow.h"
#include "gui/web/webcontext.h"

#include <DPlatformWindowHandle>
#include <DApplication>
#include <DWindowManagerHelper>

using namespace uos_ai;
DWIDGET_USE_NAMESPACE

AppWindow::AppWindow(QWidget *parent) : QMainWindow(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

void AppWindow::initWindow(WebContext *ctx)
{
    webCtx = ctx;
    ctx->appView->setParent(this);
    ctx->winCh->setWindow(this);

    setCentralWidget(ctx->appView);

    // 移除标题栏 DMainWindowPrivate
    auto noTitlebarEnabled = []{
#ifndef COMPILE_ON_QT6 // dtk5
        QFunctionPointer enableNoTitlebar = qApp->platformFunction("_d_isEnableNoTitlebar");
        bool enabled = qApp->platformName() == "dwayland" || qApp->property("_d_isDwayland").toBool();
        return DApplication::isDXcbPlatform() || (enabled && enableNoTitlebar != nullptr);
#else // dtk6
        return DApplication::isDXcbPlatform() || DWindowManagerHelper::instance()->hasNoTitlebar();
#endif
    };

    if (noTitlebarEnabled) {
        auto handle = new DPlatformWindowHandle(this, this);
        handle->setEnableSystemMove(true);

        //hasBlurWindow = DWindowManagerHelper::instance()->hasBlurWindow();
        handle->setEnableBlurWindow(true);
    }
}

