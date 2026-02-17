#include "writertextedit.h"

#include <QKeyEvent>

using namespace uos_ai;

void WriterTextEdit::keyPressEvent(QKeyEvent *e) {
    if (m_isInProgress && e->key() != Qt::Key::Key_Escape) {
        return;
    }

    QTextEdit::keyPressEvent(e);
}

void WriterTextEdit::keyReleaseEvent(QKeyEvent *e) {
    if (m_isInProgress && e->key() != Qt::Key::Key_Escape) {
        return;
    }

    QTextEdit::keyReleaseEvent(e);
}

void WriterTextEdit::mousePressEvent(QMouseEvent *e) {
    if (m_isInProgress) {
        return;
    }

    QTextEdit::mousePressEvent(e);
}

void WriterTextEdit::mouseReleaseEvent(QMouseEvent *e) {
    if (m_isInProgress) {
        return;
    }

    QTextEdit::mouseReleaseEvent(e);
}

void WriterTextEdit::mouseMoveEvent(QMouseEvent *e) {
    if (m_isInProgress) {
        return;
    }

    QTextEdit::mouseMoveEvent(e);
}

void WriterTextEdit::mouseDoubleClickEvent(QMouseEvent *e) {
    if (m_isInProgress) {
        return;
    }

    QTextEdit::mouseDoubleClickEvent(e);
}

void WriterTextEdit::contextMenuEvent(QContextMenuEvent *e) {
    if (m_isInProgress) {
        return;
    }

    QTextEdit::contextMenuEvent(e);
}
