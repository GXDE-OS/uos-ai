#ifndef CIRCLEBUTTON_H
#define CIRCLEBUTTON_H

#include "uosai_global.h"

#include <DPushButton>

namespace uos_ai {

// 圆形背景
class CircleButton : public DTK_WIDGET_NAMESPACE::DPushButton
{
    Q_OBJECT
public:
    explicit CircleButton(QWidget * parent = nullptr);
    ~CircleButton() override {}
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* e) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
#ifdef COMPILE_ON_QT6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QIcon setIconColor(QIcon icon, QColor color);

private:
    bool m_isHover            = false;
    bool m_isPress            = false;
    QIcon m_iconOrig;
};
}

#endif // CIRCLEBUTTON_H
