#ifndef WRITERTEXTEDIT_H
#define WRITERTEXTEDIT_H

#include "uosai_global.h"

#include <QTextEdit>

namespace uos_ai {

class WriterTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit WriterTextEdit(QWidget * parent = nullptr) : QTextEdit(parent) {}
    void setInProgress(bool isTrue) { m_isInProgress = isTrue; }

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    bool m_isInProgress = false;
};
}

#endif // WRITERTEXTEDIT_H
