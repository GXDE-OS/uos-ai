#ifndef FILLTEXTBUTTON_H
#define FILLTEXTBUTTON_H
#include "uosai_global.h"

#include <DPushButton>

namespace uos_ai {

class FillTextButton : public DTK_WIDGET_NAMESPACE::DPushButton
{
    Q_OBJECT
public:
    explicit FillTextButton(const QString& text, QWidget * parent = nullptr);

    QSize sizeHint() const override;
    void clearPressColor();

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent* e) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
#ifdef COMPILE_ON_QT6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

private slots:
    void onButtonReleased();

private:
    void updateRectSize();

private:
    bool m_isPress = false;
    bool m_isHover = false;
    bool m_isPressOnly = false;
};
}
#endif // FILLTEXTBUTTON_H
