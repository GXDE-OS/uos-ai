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

    m_pFileSize = new DLabel;
    m_pFileSize->setTextFormat(Qt::PlainText);
    DFontSizeManager::instance()->bind(m_pFileSize, DFontSizeManager::T8, QFont::Normal);
    m_pFileSize->setElideMode(Qt::ElideRight);
    m_pFileSize->setForegroundRole(DPalette::TextTips);

    m_pEditbutton = new IconButtonEx(this);
    m_pEditbutton->setInterruptFilter(true);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(10);
    layout->addWidget(m_pDeleteButton);
    layout->addWidget(m_pModelButton);
    layout->addWidget(m_pName);
    layout->addWidget(m_pFileSize);
    layout->addStretch();
    layout->addWidget(m_pEditbutton);
    setLayout(layout);

    setFixedSize(620, 36);
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

void OperatingLineWidget::setStatusIcon(const QString &iconName)
{
    m_pEditbutton->setStatusIcon(iconName);
}

void OperatingLineWidget::setTipsIcon(const QString &iconName)
{
    m_pEditbutton->setTipsIcon(iconName);
}

void OperatingLineWidget::setFileSize(qint64 bytes)
{
    m_fileSize = bytes;

    QString str = "";

    if (bytes < 1024)
        str = QString("%1B").arg(bytes);
    else if (bytes < 1024 * 1024)
        str =  QString("%1KB").arg(bytes / 1024.0, 0, 'f', 1);
    else if (bytes < 1024 * 1024 * 1024)
        str =  QString("%1MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    else
        str =  QString("%1GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);

    m_pFileSize->setText(str);
}

void OperatingLineWidget::setSpinnerVisible(bool visible)
{
    m_pEditbutton->setSpinnerVisible(visible);
}


void OperatingLineWidget::setStatus(KnowledgeBaseProcessStatus status)
{
    m_pEditbutton->setStatus(status);

    switch (status) {
    case Processing: {
        setEditText(tr("In data processing"));
        setStatusIcon("");
        setSpinnerVisible(true);
        QString icon = QString(":/icons/deepin/builtin/%1/icons/tip.svg");
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
            icon = icon.arg("light");
        else
            icon = icon.arg("dark");
        setTipsIcon(icon);
        break;
    }
    case ProcessingError: {
        setEditText(tr("Data processing error"));
        setStatusIcon(":/icons/deepin/builtin/warning.svg");
        setSpinnerVisible(false);
        QString icon = QString(":/icons/deepin/builtin/%1/icons/retry.svg");
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
            icon = icon.arg("light");
        else
            icon = icon.arg("dark");
        setTipsIcon(icon);
        break;
    }
    case FileError:{
        setEditText(tr("File error, unable to process, please delete."));
        setStatusIcon(":/icons/deepin/builtin/warning.svg");
        setSpinnerVisible(false);
        setTipsIcon("");
        break;
    }
    case Succeed:
    default: {
        setEditText(tr(""));
        setStatusIcon("");
        setTipsIcon("");
        setSpinnerVisible(false);
        break;
    }
    }
}
