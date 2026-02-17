#ifndef HINTTEXTEDIT_H
#define HINTTEXTEDIT_H

#include "uosai_global.h"

#include <DLabel>

#include <QTextEdit>
#include <QString>

namespace uos_ai {

// 带有hint的TextEdit
class HintTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit HintTextEdit(QWidget *parent = nullptr);
    ~HintTextEdit() override {}
    void setHint(QString text) { m_hint = text; }
    void setFirstQueryFlag(bool isfirst) { m_isFirstQuery = isfirst; }
    bool isFirstQuery() { return m_isFirstQuery; }

protected:
    void paintEvent(QPaintEvent* e) override;
    void insertFromMimeData(const QMimeData *source) override;
    void inputMethodEvent(QInputMethodEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *) override;

public:
    QRegularExpression highLightRegex;

private:
    volatile bool m_isFirstQuery  = true;
    QString m_hint;
    QString m_preeditStr;
    bool m_press = false;
    int m_startMovePos = -1;
};
}

#endif // HINTTEXTEDIT_H
