#ifndef LINENUMBERTEXTEDIT_H
#define LINENUMBERTEXTEDIT_H

#include <DTextEdit>

namespace uos_ai {
class LineNumberTextEdit : public DTK_WIDGET_NAMESPACE::DTextEdit
{
    Q_OBJECT
public:
    explicit LineNumberTextEdit(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *event) override;
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;

private:
    int lineNumberAreaWidth();
    void updateLineNumberAreaWidth();

    int m_currentNumberAreaWidth = 0;
};
}   // namespace uos_ai

#endif // LINENUMBERTEXTEDIT_H
