#include "operatinglinewidget.h"
#include "iconbuttonex.h"

#include <QHBoxLayout>
#include <QMouseEvent>

#include <DLabel>
#include <DDialog>
#include <DGuiApplicationHelper>

static constexpr char BIGMODEL[] = "uos-ai-assistant_bigmodel";

OperatingLineWidget::OperatingLineWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    initConnect();
    installEventFilter(this);
}

void OperatingLineWidget::initUI()
{
    m_pDeleteButton = new DIconButton(DStyle::SP_DeleteButton, this);
    m_pDeleteButton->setIconSize(QSize(16, 16));
    m_pDeleteButton->setFlat(true);
    m_pDeleteButton->hide();

    m_pModelButton = new DIconButton(static_cast<QStyle::StandardPixmap>(-1), this);
    m_pModelButton->setIcon(QIcon::fromTheme(BIGMODEL));
    m_pModelButton->setIconSize(QSize(13, 16));
    m_pModelButton->setFlat(true);
    m_pModelButton->hide();
    m_pModelButton->installEventFilter(this);

    m_pName = new DLabel;
    m_pName->setTextFormat(Qt::PlainText);
    DFontSizeManager::instance()->bind(m_pName, DFontSizeManager::T6, QFont::Medium);
    m_pName->setElideMode(Qt::ElideRight);

    m_pEditbutton = new IconButtonEx(this);
    m_pEditbutton->setInterruptFilter(true);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(10);
    layout->addWidget(m_pDeleteButton);
    layout->addWidget(m_pModelButton);
    layout->addWidget(m_pName);
    layout->addStretch();
    layout->addWidget(m_pEditbutton);
    setLayout(layout);

    this->setFixedHeight(36);
}

void OperatingLineWidget::initConnect()
{
    connect(m_pDeleteButton, &DIconButton::clicked, this, &OperatingLineWidget::signalDeleteButtonClicked);
    connect(m_pEditbutton, &IconButtonEx::clicked, this, &OperatingLineWidget::signalNotDeleteButtonClicked);
}

void OperatingLineWidget::setEditMode(bool edit)
{
    m_pDeleteButton->setVisible(edit);
    m_pEditbutton->setDisabled(edit);
    update();
}

void OperatingLineWidget::setName(const QString &text)
{
    m_pName->setText(text);
    m_pName->setToolTip(text);
}

void OperatingLineWidget::setEditText(const QString &text)
{
    m_pEditbutton->setText(text);
}

void OperatingLineWidget::setEditFont(DFontSizeManager::SizeType type, int weight)
{
    m_pEditbutton->setFont(type, weight);
}

void OperatingLineWidget::setEditIconSize(const QSize &size)
{
    m_pEditbutton->setIconSize(size);
}

void OperatingLineWidget::setEditHighlight(bool highlight)
{
    m_pEditbutton->setHighlight(highlight);
}

void OperatingLineWidget::setEditSpacing(int spacing)
{
    m_pEditbutton->setSpacing(spacing);
}

void OperatingLineWidget::setModelShow(bool show)
{
    show ? m_pModelButton->show() : m_pModelButton->hide();
}

bool OperatingLineWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && !m_bInterrupt) {
        auto mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            emit signalNotDeleteButtonClicked();
        }
    }

    return DWidget::eventFilter(obj, event);
}

void OperatingLineWidget::setInterruptFilter(bool interrupt)
{
    m_bInterrupt = interrupt;
}
