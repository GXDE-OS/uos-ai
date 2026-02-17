#include "querytextedit.h"
#include "../gui/aiquickdialog.h"
#include "../gui/aiwriterdialog.h"

#include <DGuiApplicationHelper>

#include <QFontMetrics>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

QueryTextEdit::QueryTextEdit(QWidget *parent) : QTextEdit(parent), m_parent(parent) {
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, &QueryTextEdit::adjustText);
    connect(this, &QueryTextEdit::textChanged, this, [&] {
        this->asyncAdjustHeight(100);
    });
}

void QueryTextEdit::setFullText(const QString &text) {
    m_fullText = text;
    this->adjustText();
}

void QueryTextEdit::showEvent(QShowEvent *e) {
    QTextEdit::showEvent(e);
    this->asyncAdjustHeight(100);
}

void QueryTextEdit::paintEvent(QPaintEvent *e) {
    QTextEdit::paintEvent(e);
}

void QueryTextEdit::mouseDoubleClickEvent(QMouseEvent *e) {
    if (m_parent && dynamic_cast<AiQuickDialog *>(m_parent)) {
        dynamic_cast<AiQuickDialog *>(m_parent)->copyText(m_fullText);
    } else if (m_parent && dynamic_cast<AiWriterDialog *>(m_parent)) {
        dynamic_cast<AiWriterDialog *>(m_parent)->copyText(m_fullText);
    }
}

void QueryTextEdit::adjustText() {
    QFontMetrics fm(this->font());
    int width = this->width() * 2 - fm.horizontalAdvance("，") * 4;
    QString tempStr = m_fullText;
    tempStr.replace("\n", " ");
    if (fm.horizontalAdvance(tempStr) < width) {
        this->setPlainText(tempStr);
        return;
    }

    // …
    this->setPlainText(fm.elidedText(tempStr, Qt::ElideRight, width));

    QTextBlockFormat blockFmt = this->textCursor().blockFormat();
    blockFmt.setLineHeight(122, QTextBlockFormat::LineHeightTypes::ProportionalHeight);
    this->textCursor().setBlockFormat(blockFmt);
}

void QueryTextEdit::asyncAdjustHeight(int ms) {
    QTimer::singleShot(ms, this, [&] {
        if (int(this->document()->size().height()) == 0) {
            return;
        }

        this->setFixedHeight(int(this->document()->size().height()) + 2);
        // 获取行数，一行字取15像素的高度，两行字取35像素的高
        int lineCount = int(this->document()->size().height() / (QFontMetrics(this->font()).height() * 1.22) + 0.5);
        if (m_parent && dynamic_cast<AiQuickDialog *>(m_parent)) {
            dynamic_cast<AiQuickDialog *>(m_parent)->setQuerySepHeight(lineCount == 1 ? 15 : 35);
        } else if (m_parent && dynamic_cast<AiWriterDialog *>(m_parent)) {
            dynamic_cast<AiWriterDialog *>(m_parent)->setQuerySepHeight(lineCount == 1 ? 15 : 35);
        }
    });
}
