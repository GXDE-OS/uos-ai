#ifndef QUERYTEXTEDIT
#define QUERYTEXTEDIT

#include "uosai_global.h"

#include <QTextEdit>

namespace uos_ai {
class QueryTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit QueryTextEdit(QWidget *parent = nullptr);
    void setFullText(const QString &text);

protected:
    void showEvent(QShowEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;

private:
    void adjustText();
    void asyncAdjustHeight(int ms = 0);

private:
    QWidget *m_parent = nullptr;
    QString m_fullText;
};
}

#endif // QUERYTEXTEDIT
