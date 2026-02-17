#ifndef SENDBUTTON_H
#define SENDBUTTON_H

#include <DIconButton>
#include <QMouseEvent>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

namespace uos_ai {

class SendButton : public DIconButton
{
    Q_OBJECT

public:
    explicit SendButton(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
#ifdef COMPILE_ON_QT6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

private:
    bool m_isHover = false;
    bool m_isPress = false;
};

}

#endif // SENDBUTTON_H 