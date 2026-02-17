#ifndef INPUTPLACEHOLDERWIDGET_H
#define INPUTPLACEHOLDERWIDGET_H

#include <DWidget>
#include <QLabel>
#include <QHBoxLayout>

namespace uos_ai {

class InputPlaceholderWidget : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit InputPlaceholderWidget(QWidget *parent = nullptr);

signals:
    void clicked();

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
    void updateTheme();

    QLabel *m_textLabel;
    bool m_isPressed;
    bool m_isHovered;
    bool m_hasBeenClicked;  // 添加标记是否已经点击过
};

}

#endif // INPUTPLACEHOLDERWIDGET_H 
