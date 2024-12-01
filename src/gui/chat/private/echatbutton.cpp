// SPDX-FileCopyrightText: 2015 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "echatbutton.h"

#include <DStyleOption>
#include <DGuiApplicationHelper>

#include <QSvgRenderer>
#include <QFile>

EChatButton::EChatButton(QWidget * parent)
    : DIconButton(parent)
{
    //QStyle::SP_TitleBarCloseButton
//    DStyledIconEngine *iconEngine = new DStyledIconEngine(DDrawUtils::drawTitleBarNormalButton, QString(":/icons/deepin/builtin/dark/icons/chat.svg"));
//    setIcon(QIcon(iconEngine));
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    setFlat(true);
}

QSize EChatButton::sizeHint() const
{
    return iconSize();
}

void EChatButton::initStyleOption(DStyleOptionButton *option) const
{
    DIconButton::initStyleOption(option);

    option->features |= QStyleOptionButton::ButtonFeature(DStyleOptionButton::TitleBarButton);
}

void EChatButton::paintEvent(QPaintEvent* e)
{
    QRectF rect = this->rect();
    QPainter pa(this);
    QColor backgroundColor = Qt::transparent;
    QPalette palette = this->palette();
    //qDebug() << this->palette().foreground().color().alpha();

    if (isEnabled() && m_isHover) {
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType)
            backgroundColor = QColor(255, 255, 255, 13);
        else
            backgroundColor = QColor(0, 0, 0, 13);
    }

    pa.setPen(Qt::NoPen);
    pa.setBrush(QBrush(backgroundColor));
    pa.drawRect(rect);

    // 绘制图标
    double content_width = rect.width() / 10 * 3;
    double content_height = rect.height() / 5;
    QRectF content_rect(0, 0, content_width, content_height);
    content_rect.moveCenter(rect.center());

//    pa.setPen(Qt::red);
//    pa.drawRect(content_rect);

    //QList<int> heights = {13, 20, 30, 18, 11};
    QList<double> heights = {content_rect.height() / 10 * 4 + 0.5,
                          content_rect.height() / 10 * 7 + 0.5,
                          content_rect.height() / 10 * 11 + 0.5,
                          content_rect.height() / 10 * 6 + 0.5,
                          content_rect.height() / 10 * 3 + 0.5 };

    QColor color = this->palette().buttonText().color();
    if (m_isPress || m_isActive)
        color = this->palette().highlight().color();

    pa.setPen(color);
    pa.setRenderHint(QPainter::Antialiasing, pa.device()->devicePixelRatioF() > 1.0);
    int numLines = heights.size();
    double spaceBetweenLines = (content_width - numLines) / (numLines - 1) + 0.5;
    for (int i = 0; i < numLines; ++i) {
        double x = content_rect.left() + i * spaceBetweenLines + i;
        double y = content_rect.top() + (content_height - heights[i]) / 2 + 0.5;
        QPointF top(x, y);
        QPointF bottom(x, y + heights[i]);
        pa.drawLine(top, bottom);
        //qDebug() << "height: " << heights[i];
    }
}
void EChatButton::mousePressEvent(QMouseEvent *e)
{
    m_isPress = true;
    update();

    return DIconButton::mousePressEvent(e);
}
void EChatButton::mouseReleaseEvent(QMouseEvent *e)
{
    m_isPress = false;
    update();
    return DIconButton::mouseReleaseEvent(e);
}
void EChatButton::enterEvent(QEvent *event)
{
    m_isHover = true;
    update();

    return DIconButton::enterEvent(event);
}
void EChatButton::leaveEvent(QEvent *event)
{
    m_isHover = false;
    update();
    return DIconButton::leaveEvent(event);
}

QPixmap EChatButton::getIcon()
{
    QColor color = this->palette().buttonText().color();

    if (m_isPress || m_isActive)
        color = this->palette().highlight().color();

    QSize iconSize(20, 20);
    /*QPixmap pixmap = QPixmap(iconSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(color);
    m_icon.paint(&painter, QRect(0, 0, iconSize.width(), iconSize.height()),
                                               Qt::AlignCenter, QIcon::Active, QIcon::On);*/
    QImage image = m_icon.pixmap(iconSize, QIcon::Active, QIcon::On).toImage();
    for (int i = 0; i < image.width(); i++) {
        for (int j = 0; j < image.height(); j++) {
            if (image.pixelColor(i, j) != QColor(0, 0, 0, 0)) {
                image.setPixelColor(i, j, color);
            }
        }
    }
    QPixmap pixmap = QPixmap::fromImage(image);

    return pixmap;
}

void EChatButton::updateActiveStatus(bool isActive)
{
    m_isActive = isActive; update();
    if (m_isActive)
        setToolTip(tr("Turn off voice conversation"));
    else
        setToolTip(tr("Voice conversation"));
}

