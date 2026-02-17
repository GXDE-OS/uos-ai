#ifndef WIZARDDPUSHBUTTON
#define WIZARDDPUSHBUTTON
#include "uosai_global.h"

#include <DPushButton>

namespace uos_ai {

class LIBDTKWIDGETSHARED_EXPORT WizardDPushButton : public DTK_WIDGET_NAMESPACE::DPushButton
{
    Q_OBJECT
public:
    WizardDPushButton(QWidget * parent = nullptr);
    WizardDPushButton(const QString& text, QWidget * parent = nullptr);

    void setHoverStatus(bool isHover);

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
    void updateRectSize();

private slots:
    void onButtonReleased();

private:
    bool m_isHover = false;
    bool m_isPress = false;
    int m_maxWidth = 180; // 最大宽度10中文字符宽度

};
}

#endif // WIZARDDPUSHBUTTON
