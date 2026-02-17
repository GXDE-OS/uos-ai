#include "hinttextedit.h"
#include "utils/util.h"
#include "xclipboard.h"

#include <QMimeData>
#include <QInputMethodEvent>
#include <QClipboard>
#include <QTimer>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
DCORE_USE_NAMESPACE
using namespace uos_ai;

HintTextEdit::HintTextEdit(QWidget *parent) : QTextEdit(parent),
    highLightRegex(R"(【(.*?)】)", QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption)
{
    if (!UOSAI_NAMESPACE::Util::checkLanguage())
        highLightRegex.setPattern(R"(\[(.*?)\])");
}

void HintTextEdit::paintEvent(QPaintEvent* e) {
    QTextEdit::paintEvent(e);
    if (this->toPlainText().isEmpty() && m_preeditStr.isEmpty() && !m_hint.isEmpty()) {
        QRectF rect = this->rect();
        QPainter pa(this->viewport());
        QColor textColor = this->palette().color(DPalette::Normal, DPalette::WindowText);
        textColor.setAlphaF(0.3);
        pa.setPen(textColor);
        QTextOption textOption(Qt::AlignLeft);
        textOption.setWrapMode(QTextOption::NoWrap);
        rect.setX(rect.x() + 5);
        rect.setY(rect.y() + 3);
        pa.drawText(rect, m_hint, textOption);
    }
}

void HintTextEdit::insertFromMimeData(const QMimeData *source) {
    // 只要纯文本
    if (!source->hasText()) {
        return;
    }

    QMimeData textData;
    textData.setText(m_isFirstQuery ? source->text() : source->text().replace("\n", ""));
    QTextEdit::insertFromMimeData(&textData);
}

void HintTextEdit::inputMethodEvent(QInputMethodEvent *e) {
    // 处理输入法的预编辑字符串
    m_preeditStr = e->preeditString();

    QTextEdit::inputMethodEvent(e);
}

void HintTextEdit::mousePressEvent(QMouseEvent *event) {
    QTextEdit::mousePressEvent(event);
    QTextCursor cursor = cursorForPosition(event->pos());

    int pos = cursor.position();

    QRegularExpressionMatchIterator matchIterator = highLightRegex.globalMatch(toPlainText());

    BaseClipboard::instance()->blockChangedSignal(true); // 禁用信号

    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        if (match.capturedStart() < pos && match.capturedEnd() > pos) {
            cursor.setPosition(match.capturedStart() + 1);
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength() - 2);
            setTextCursor(cursor);
        }
    }
    m_press = true;
}

void HintTextEdit::mouseMoveEvent(QMouseEvent *event)
{
    if (m_press) {
        QTextCursor cursor = cursorForPosition(event->pos());
        int pos = cursor.position();
        if (m_startMovePos < 0)
            m_startMovePos = pos;
        cursor.setPosition(m_startMovePos);
        cursor.setPosition(pos, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
        QTextEdit::mouseMoveEvent(event);
    }
}

void HintTextEdit::mouseReleaseEvent(QMouseEvent *event)
{
    m_startMovePos = -1;
    m_press = false;
    QTextEdit::mouseReleaseEvent(event);
}

void HintTextEdit::leaveEvent(QEvent *e)
{
    BaseClipboard::instance()->blockChangedSignal(false);
    return QTextEdit::leaveEvent(e);
}
