#include "backgroundframe.h"

#include <DGuiApplicationHelper>

#include <QPainter>

BackgroundFrame::BackgroundFrame(QWidget *parent)
    : DFrame(parent)
    , m_isDarkTheme(false)
{
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &BackgroundFrame::updateSystemTheme);
    updateSystemTheme(DGuiApplicationHelper::instance()->themeType());
}

BackgroundFrame::~BackgroundFrame()
{
}

void BackgroundFrame::updateSystemTheme(const DGuiApplicationHelper::ColorType &themeType)
{
    if (themeType == DGuiApplicationHelper::DarkType) {
        m_isDarkTheme = true;
    } else {
        m_isDarkTheme = false;
    }
}

void BackgroundFrame::paintEvent(QPaintEvent *)
{
    if (!m_isDarkTheme) {
        paintLight();
    } else {
        paintDark();
    }
}

/**
 * @brief 绘制浅色主题样式
 */
void BackgroundFrame::paintLight()
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setBrush(QColor(255, 255, 255, 128));
    QPen pen;
    pen.setColor(QColor(0, 0, 0, 25));
    pen.setWidth(1);
    painter.setPen(pen); //设置画笔记颜色
    painter.drawRoundedRect(1, 1, width() - 2, height() - 2, 8, 8);
}

/**
 * @brief 绘制深色主题样式
 */
void BackgroundFrame::paintDark()
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setBrush(QColor(0, 0, 0, 13));
    QPen pen;
    pen.setColor(QColor(255, 255, 255, 25));
    pen.setWidth(1);
    painter.setPen(pen); //设置画笔记颜色
    painter.drawRoundedRect(1, 1, width() - 2, height() - 2, 8, 8);
}
