#include "topdialog.h"

#include <utils/esystemcontext.h>

#include <DPalette>
#include <DGuiApplicationHelper>

#ifdef COMPILE_ON_V25
#include <ddeshellwayland.h>
#endif

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

TopDialog::TopDialog(QWidget *parent) : DDialog(parent) {
    this->setWindowFlags(this->windowFlags() | Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint);
    m_titleBar = this->findChild<DTitlebar *>();
    if (m_titleBar) {
        m_titleBar->installEventFilter(this);
    }

#ifdef COMPILE_ON_V25
    if (ESystemContext::isTreeland()) {
        this->createWinId();
        DDEShellWayland::get(windowHandle())->setRole(QtWayland::treeland_dde_shell_surface_v1::role_overlay);
    }
#endif
}

bool TopDialog::eventFilter(QObject *watched, QEvent *event) {
    if (!ESystemContext::isTreeland()) {
        // 取消窗管控制后，自己实现拖拽行为
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(event);
            if (event->type() == QEvent::MouseButtonPress) {
                this->mousePressEvent(mouseEvent);
            } else if (event->type() == QEvent::MouseButtonRelease) {
                this->mouseReleaseEvent(mouseEvent);
            } else if (event->type() == QEvent::MouseMove) {
                this->mouseMoveEvent(mouseEvent);
            }
        }
    }

    return DDialog::eventFilter(watched, event);
}

void TopDialog::mousePressEvent(QMouseEvent *event)
{
    if (!ESystemContext::isTreeland()) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            return;
        }
    }

    DDialog::mousePressEvent(event);
}

void TopDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (!ESystemContext::isTreeland()) {
        if (m_dragging) {
            this->setCursor(Qt::CursorShape::SizeAllCursor);
            this->move(event->globalPos() - m_dragStartPos);
            return;
        }
    }

    DDialog::mouseMoveEvent(event);
}

void TopDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (!ESystemContext::isTreeland()) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            this->setCursor(Qt::CursorShape::ArrowCursor);
            return;
        }
    }

    DDialog::mouseReleaseEvent(event);
}
