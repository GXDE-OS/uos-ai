#ifndef EMASKWIDGET_H
#define EMASKWIDGET_H

#include <QWidget>
#include <QPainter>

class EMaskWidget : public QWidget
{
public:
    explicit EMaskWidget(QWidget *parent = nullptr);
    void setBackGroundColor(QColor color);

protected:
    void paintEvent(QPaintEvent* event) override ;
private:
    QColor m_backgroundColor = QColor(0, 0, 0, 180);
};

#endif // EMASKWIDGET_H
