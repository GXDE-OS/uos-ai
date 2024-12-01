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
    layout->setSpacing(5);

    m_pStatusIcon = new DLabel;
    m_pStatusIcon->setPixmap(QPixmap(":/icons/deepin/builtin/loading.svg"));
    m_pStatusIcon->hide();

    m_pLabel = new DLabel(text);
    DFontSizeManager::instance()->bind(m_pLabel, DFontSizeManager::T8, QFont::Normal);
    m_pLabel->setForegroundRole(DPalette::TextTips);

    m_pTipsIcon = new DLabel;
    QString icon = QString(":/icons/deepin/builtin/%1/icons/tip.svg");
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
        icon = icon.arg("light");
    else
        icon = icon.arg("dark");
    m_pTipsIcon->setPixmap(QPixmap(icon));
    m_pTipsIcon->installEventFilter(this);
    m_pTipsIcon->hide();

    m_pIcon = new DLabel;
    QPixmap pixmap = QApplication::style()->standardIcon(QStyle::SP_ArrowForward).pixmap(m_iconSize);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), DGuiApplicationHelper::instance()->applicationPalette().textTips());
    m_pIcon->setPixmap(pixmap);

    m_spinner = new DSpinner(this);
    m_spinner->setFixedSize(12, 12);
    m_spinner->hide();

    layout->addWidget(m_spinner);
    layout->addWidget(m_pStatusIcon);
    layout->addWidget(m_pLabel);
    layout->addWidget(m_pTipsIcon);
    layout->addWidget(m_pIcon);

    installEventFilter(this);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &IconButtonEx::onThemeTypeChanged);
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
    if (m_bHighlight) {
        m_pLabel->setForegroundRole(DPalette::Highlight);

        QPixmap pixmap = QApplication::style()->standardIcon(QStyle::SP_ArrowForward).pixmap(m_iconSize);
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), DGuiApplicationHelper::instance()->applicationPalette().highlight());
        m_pIcon->setPixmap(pixmap);
    }
}

void IconButtonEx::setSpacing(int spacing)
{
    this->layout()->setSpacing(spacing);
}

void IconButtonEx::paintEvent(QPaintEvent *event)
{
    DWidget::paintEvent(event);
}

bool IconButtonEx::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && !m_bInterrupt) {
        auto mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            emit clicked();
        }
    }

    if (event->type() == QEvent::Enter) {
        if (obj == m_pTipsIcon) {
            QPoint p = mapToGlobal(m_pTipsIcon->pos());
            showTips(p.x() + 20, p.y() + 15);
        }
    } else if (event->type() == QEvent::Leave) {
        if (obj == m_pTipsIcon) {
            hideTips();
        }
    }

    return DWidget::eventFilter(obj, event);
}

bool IconButtonEx::event(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange) {
        QMetaObject::invokeMethod(this, "onApplicationPaletteChanged", Qt::QueuedConnection);
    }
    return DWidget::event(event);
}

void IconButtonEx::setInterruptFilter(bool interrupt)
{
    m_bInterrupt = interrupt;
}

void IconButtonEx::setStatusIcon(const QString &iconName)
{
    if (iconName.isEmpty()) {
        m_pStatusIcon->hide();
    } else {
        m_pStatusIcon->setPixmap(QPixmap(iconName));
        m_pStatusIcon->show();
    }
}

void IconButtonEx::setTipsIcon(const QString &iconName)
{
    if (iconName.isEmpty()) {
        m_pTipsIcon->hide();
    } else {
        m_pTipsIcon->setPixmap(QPixmap(iconName));
        m_pTipsIcon->show();
    }
}

void IconButtonEx::setSpinnerVisible(bool visible)
{
    if (visible) {
        m_spinner->show();
        m_spinner->start();
    } else {
        m_spinner->stop();
        m_spinner->hide();
    }
}

void IconButtonEx::onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType)
{
    if (!m_bHighlight) {
        QPixmap pixmap = QApplication::style()->standardIcon(QStyle::SP_ArrowForward).pixmap(m_iconSize);
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), DGuiApplicationHelper::instance()->applicationPalette().textTips());
        m_pIcon->setPixmap(pixmap);
    }

    if (DGuiApplicationHelper::LightType == themeType) {

    } else {

    }
}

void IconButtonEx::onApplicationPaletteChanged()
{
    if (m_bHighlight) {
        m_pLabel->setForegroundRole(DPalette::Highlight);

        QPixmap pixmap = QApplication::style()->standardIcon(QStyle::SP_ArrowForward).pixmap(m_iconSize);
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), DGuiApplicationHelper::instance()->applicationPalette().highlight());
        m_pIcon->setPixmap(pixmap);
    }
}

void IconButtonEx::showTips(int x, int y)
{
    if (m_knowledgeBaseProcessStatus != KnowledgeBaseProcessStatus::Processing)
        return;

    if (!m_tips) {
        m_tips = new DLabel();
        m_tips->setText(tr("Newly added files require preprocessing before they can be used, and during the data processing process, "
                           "it may consume a significant amount of computing and storage resources. Data processing will stop after deleting data."));
        m_tips->setWordWrap(true);
        m_tips->setFixedWidth(320);
        m_tips->setContentsMargins(20, 15, 17, 15);
        QPalette pl = m_tips->palette();
        pl.setColor(QPalette::Text, DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Text));
        m_tips->setPalette(pl);
        m_tips->setForegroundRole(QPalette::Text);
        m_tips->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_tips->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::Window);
        m_tips->setForegroundRole(DPalette::TextTitle);
        DFontSizeManager::instance()->bind(m_tips, DFontSizeManager::T8, QFont::Medium);

        m_tips->move(x, y);
        m_tips->show();
    } else {
        m_tips->move(x, y);
        m_tips->show();
    }
}
void IconButtonEx::hideTips()
{
    if (m_tips)
        m_tips->hide();
}


void IconButtonEx::setStatus(KnowledgeBaseProcessStatus status)
{
    m_knowledgeBaseProcessStatus = status;
}

