#ifndef TOPDIALOG
#define TOPDIALOG

#include "uosai_global.h"

#include <DDialog>
#include <DTitlebar>

#include <QEvent>
#include <QMouseEvent>

namespace uos_ai {
class TopDialog : public DTK_WIDGET_NAMESPACE::DDialog
{
    Q_OBJECT
public:
    explicit TopDialog(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    DTK_WIDGET_NAMESPACE::DTitlebar *m_titleBar = nullptr;

    bool m_dragging = false;
    QPoint m_dragStartPos;
};
}

#endif // TOPDIALOG
