#include "wrapcheckbox.h"

#include <DFontSizeManager>
#include <DGuiApplicationHelper>

#include <QApplication>
#include <QHBoxLayout>

WrapCheckBox::WrapCheckBox(QWidget *parent) : QWidget(parent)
{
    initUI();
    initConnect();
}

WrapCheckBox::WrapCheckBox(const QString &text, QWidget *parent)
    : QWidget(parent)
{
    initUI();
    initConnect();

    m_label->setText(text);
}

Qt::CheckState WrapCheckBox::checkState() const
{
    if (m_checkBox)
        return m_checkBox->checkState();

    return Qt::CheckState::Unchecked;
}

void WrapCheckBox::setCheckState(Qt::CheckState state)
{
    if (m_checkBox)
        m_checkBox->setCheckState(state);
}

void WrapCheckBox::setText(const QString &text)
{
    if (!m_label) return;

    m_text = text;
    onUpdateSystemFont(QFont());
}

QString WrapCheckBox::text() const
{
    if (m_label)
        return m_label->text();

    return QString();
}

void WrapCheckBox::initUI()
{
    m_checkBox = new DCheckBox;
    m_checkBox->setFixedSize(QSize(20, 20));

    m_label = new DLabel;
    m_label->setWordWrap(false);
    m_label->setForegroundRole(QPalette::Text);
    DFontSizeManager::instance()->bind(m_label, DFontSizeManager::T6, QFont::Medium);

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(m_checkBox, 0, Qt::AlignTop | Qt::AlignLeft);
    layout->addWidget(m_label, 0, Qt::AlignTop | Qt::AlignLeft);
    layout->addStretch();

    this->setLayout(layout);
}

void WrapCheckBox::initConnect()
{
    connect(m_checkBox, &DCheckBox::stateChanged, this, &WrapCheckBox::stateChanged);
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));
}

void WrapCheckBox::onUpdateSystemFont(const QFont &)
{
    // 富文本不能通过计算换行，改为自动换行
    if (m_label->textFormat() == Qt::RichText) {
        m_label->setText(m_text);
        m_label->setWordWrap(true);
        return;
    }

    int maxWidth = m_maxWidth - 10;
    QFontMetrics fontMetrics(m_label->font());

    if (fontMetrics.width(m_text) > maxWidth) {
        QString wrappedText;
        int startPos = 0;
        int endPos = 0;

        while (endPos < m_text.length()) {
            endPos = startPos + 1;
            while (fontMetrics.width(m_text.mid(startPos, endPos - startPos)) <= maxWidth && endPos < m_text.length()) {
                endPos++;
            }
            wrappedText += m_text.mid(startPos, endPos - startPos).trimmed();
            if (endPos < m_text.length()) {
                wrappedText += "\n";
            }
            startPos = endPos;
        }

        m_label->setText(wrappedText);
    } else {
        m_label->setText(m_text);
    }
}

Qt::TextFormat WrapCheckBox::textFormat() const
{
    if (m_label)
        return m_label->textFormat();

    return Qt::AutoText;
}

void WrapCheckBox::setTextFormat(Qt::TextFormat format)
{
    if (m_label)
        m_label->setTextFormat(format);
}

bool WrapCheckBox::openExternalLinks() const
{
    if (m_label)
        return m_label->openExternalLinks();

    return false;
}

void WrapCheckBox::setOpenExternalLinks(bool open)
{
    if (m_label)
        m_label->setOpenExternalLinks(open);
}

void WrapCheckBox::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    if (m_label)
        m_label->setTextInteractionFlags(flags);
}

Qt::TextInteractionFlags WrapCheckBox::textInteractionFlags() const
{
    if (m_label)
        return m_label->textInteractionFlags();

    return Qt::LinksAccessibleByMouse;
}

void WrapCheckBox::setTextMaxWidth(int width)
{
    if (m_label->textFormat() == Qt::RichText) {
        m_label->setFixedWidth(width);
    }
    m_maxWidth = width;
}

void WrapCheckBox::setDisabled(bool b)
{
    if (m_checkBox)
        m_checkBox->setDisabled(b);
}
