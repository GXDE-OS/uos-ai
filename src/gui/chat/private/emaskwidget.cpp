#include "emaskwidget.h"

EMaskWidget::EMaskWidget(QWidget *parent) : QWidget(parent)
{
//    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    this->setAttribute(Qt::WA_TranslucentBackground);
}

void EMaskWidget::setBackGroundColor(QColor color)
{
    m_backgroundColor = color;
}

void EMaskWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(this->rect(), m_backgroundColor); // 绘制半透明背景

    return QWidget::paintEvent(event);
}
