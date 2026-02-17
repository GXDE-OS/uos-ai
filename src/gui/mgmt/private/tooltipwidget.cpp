#include "tooltipwidget.h"

#include <QPainter>
#include <QStyleOption>
#include <QGraphicsDropShadowEffect>

DGUI_USE_NAMESPACE
using namespace uos_ai;

TooltipWidget::TooltipWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::MSWindowsFixedSizeDialogHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);

    // 阴影效果
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(20);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(6);
    shadowEffect->setColor(QColor(0, 0, 0, int(255 * 0.1))); // (不透明度约10%)
    this->setGraphicsEffect(shadowEffect);
}

void TooltipWidget::paintEvent(QPaintEvent *event)
{
    QRect rect = this->rect();
    QPainter pa(this);
    pa.setRenderHints(QPainter::Antialiasing);
    QColor backgroundColor;
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        backgroundColor = QColor(247, 247, 247, 255);
    } else {
        backgroundColor = QColor(42, 42, 42, 255);
    }
    QColor borderColor;
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        borderColor = QColor(0, 0, 0, int(255 * 0.05));
    } else {
        borderColor = QColor(0, 0, 0, int(255 * 0.3));
    }

    pa.setPen(borderColor);
    pa.setBrush(QBrush(backgroundColor));
    pa.drawRoundedRect(10, 10, rect.width() - 20, rect.height() - 20, 8, 8);
}
