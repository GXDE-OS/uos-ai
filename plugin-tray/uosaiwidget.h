#ifndef UOSAIWIDGET_H
#define UOSAIWIDGET_H

#include <constants.h>

#include <QWidget>
#include <QIcon>

class QLabel;
class UosAiWidget: public QWidget
{
    Q_OBJECT

public:
    explicit UosAiWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool containCursorPos();
    void loadSvg();
private:
    bool m_hover;
    bool m_pressed;
    QPixmap m_pixmap;
};

#endif // UOSAIWIDGET_H
