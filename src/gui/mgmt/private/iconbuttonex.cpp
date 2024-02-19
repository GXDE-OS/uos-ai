#include "iconbuttonex.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QApplication>

//#include <DPaletteHelper>
#include <DGuiApplicationHelper>

IconButtonEx::IconButtonEx(DWidget *parent)
    : IconButtonEx("", parent)
{
}

IconButtonEx::IconButtonEx(const QString text, DWidget *parent)
    : DWidget(parent)
    , m_iconSize(QSize(12, 12))
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_pLabel = new DLabel(text);
    DFontSizeManager::instance()->bind(m_pLabel, DFontSizeManager::T8, QFont::Normal);
    m_pLabel->setForegroundRole(DPalette::TextTips);

    m_pIcon = new DLabel;
    QPixmap pixmap = QApplication::style()->standardIcon(QStyle::SP_ArrowForward).pixmap(m_iconSize);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), DGuiApplicationHelper::instance()->applicationPalette().textTips());
    m_pIcon->setPixmap(pixmap);

    layout->addWidget(m_pLabel);
    layout->addWidget(m_pIcon);

    installEventFilter(this);
}

void IconButtonEx::setText(const QString &text)
{
    m_pLabel->setText(text);
}

void IconButtonEx::setFont(DFontSizeManager::SizeType type, int weight)
{
    DFontSizeManager::instance()->bind(m_pLabel, type, weight);
}

void IconButtonEx::setIconSize(const QSize &size)
{
    m_iconSize = size;
}

void IconButtonEx::setHighlight(bool highlight)
{
    m_bHighlight = highlight;
}

void IconButtonEx::setSpacing(int spacing)
{
    this->layout()->setSpacing(spacing);
}

void IconButtonEx::paintEvent(QPaintEvent *event)
{
    DWidget::paintEvent(event);

    if (!m_bHighlight) return;

    m_pLabel->setForegroundRole(DPalette::Highlight);

    QPixmap pixmap = QApplication::style()->standardIcon(QStyle::SP_ArrowForward).pixmap(m_iconSize);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), DGuiApplicationHelper::instance()->applicationPalette().highlight());
    m_pIcon->setPixmap(pixmap);
}

bool IconButtonEx::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && !m_bInterrupt) {
        auto mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            emit clicked();
        }
    }

    return DWidget::eventFilter(obj, event);
}

void IconButtonEx::setInterruptFilter(bool interrupt)
{
    m_bInterrupt = interrupt;
}
