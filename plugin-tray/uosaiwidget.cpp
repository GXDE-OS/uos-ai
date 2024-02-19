// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uosaiwidget.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QIcon>
#include <QPainterPath>
#include <QLabel>
#include <QVBoxLayout>

#include <DGuiApplicationHelper>
#include <DStyle>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

UosAiWidget::UosAiWidget(QWidget *parent)
    : QWidget(parent)
    , m_hover(false)
    , m_pressed(false)
{
    setMouseTracking(true);
    setMinimumSize(PLUGIN_BACKGROUND_MIN_SIZE, PLUGIN_BACKGROUND_MIN_SIZE);
    loadSvg();
}

void UosAiWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QPainter painter(this);

    const auto ratio = devicePixelRatioF();
    const QRectF &rf = QRectF(rect());
    const QRectF &rfp = QRectF(m_pixmap.rect());
    QPointF center = rf.center() - rfp.center() / ratio;
    painter.drawPixmap(center, m_pixmap);
}

void UosAiWidget::resizeEvent(QResizeEvent *event)
{
    loadSvg();
    update();
    QWidget::resizeEvent(event);
}

void UosAiWidget::mousePressEvent(QMouseEvent *event)
{
    if (containCursorPos()) {
        m_pressed = true;
    } else {
        m_pressed = false;
    }

    update();

    QWidget::mousePressEvent(event);
}

void UosAiWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_pressed = false;
    m_hover = false;
    update();

    QWidget::mouseReleaseEvent(event);
}

void UosAiWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (containCursorPos()) {
        m_hover = true;
    } else {
        m_hover = false;
    }

    QWidget::mouseMoveEvent(event);
}

void UosAiWidget::leaveEvent(QEvent *event)
{
    m_hover = false;
    m_pressed = false;
    update();

    QWidget::leaveEvent(event);
}

bool UosAiWidget::containCursorPos()
{
    QPoint cursorPos = this->mapFromGlobal(QCursor::pos());
    QRect rect(this->rect());

    int iconSize = qMin(rect.width(), rect.height());
    int w = (rect.width() - iconSize) / 2;
    int h = (rect.height() - iconSize) / 2;

    rect = rect.adjusted(w, h, -w, -h);

    return rect.contains(cursorPos);
}

void UosAiWidget::loadSvg()
{
    const auto ratio = devicePixelRatioF();
    const int maxSize = PLUGIN_BACKGROUND_MAX_SIZE - 8;

    const int maxWidth = qMin(maxSize, this->width());
    const int maxHeight = qMin(maxSize, this->height());

    const int maxPixSize = qMin(maxWidth, maxHeight);

    QSize iconSize(maxPixSize, maxPixSize);
    m_pixmap = QPixmap(int(iconSize.width() * ratio), int(iconSize.height() * ratio));
    QSvgRenderer renderer(QString(":/images/uos-ai-assistant.svg"));
    m_pixmap.fill(Qt::transparent);

    QPainter painter;
    painter.begin(&m_pixmap);
    renderer.render(&painter);
    painter.end();
    m_pixmap.setDevicePixelRatio(ratio);
}
