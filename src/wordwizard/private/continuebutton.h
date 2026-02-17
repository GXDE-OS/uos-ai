#ifndef CONTINUEBUTTON
#define CONTINUEBUTTON

#include "uosai_global.h"

#include <DPushButton>

namespace uos_ai {
class ContinueButton : public DTK_WIDGET_NAMESPACE::DPushButton
{
    Q_OBJECT
public:
    explicit ContinueButton(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
#ifdef COMPILE_ON_QT6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    bool m_isHover            = false;
    bool m_isPress            = false;
};
}

#endif // CONTINUEBUTTON
