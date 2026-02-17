#ifndef ICONCOMMANDLINKBUTTON_H
#define ICONCOMMANDLINKBUTTON_H
#include "uosai_global.h"
#include <DCommandLinkButton>

namespace uos_ai {

enum class IconPosition {
    Left,   // 图标在文字左边
    Right   // 图标在文字右边
};

class IconCommandLinkButton : public DTK_WIDGET_NAMESPACE::DCommandLinkButton
{
    Q_OBJECT
public:
    explicit IconCommandLinkButton(const QString &text, IconPosition iconPos = IconPosition::Left, QWidget *parent = nullptr);

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *e) override;
#ifdef COMPILE_ON_QT6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

private:
    bool m_isHover = false;
    IconPosition m_iconPosition;

};
}

#endif // ICONCOMMANDLINKBUTTON_H
