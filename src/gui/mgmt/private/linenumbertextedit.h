#ifndef LINENUMBERTEXTEDIT_H
#define LINENUMBERTEXTEDIT_H

#include <DTextEdit>

namespace uos_ai {
class LineNumberTextEdit : public DTK_WIDGET_NAMESPACE::DTextEdit
{
    Q_OBJECT
public:
    explicit LineNumberTextEdit(QWidget *parent = nullptr);

    // 设置是否启用花括号连接线
    void setEnableBracketGuides(bool enable) {
        if (m_enableBracketGuides != enable) {
            m_enableBracketGuides = enable;
            viewport()->update();
        }
    }
    bool enableBracketGuides() const { return m_enableBracketGuides; }

protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *event) override;
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;

private:
    int lineNumberAreaWidth();
    void updateLineNumberAreaWidth();

    // 花括号连接线相关
    struct BracketPair {
        int openBlockNumber;   // 开括号所在块号
        int closeBlockNumber;  // 闭括号所在块号
        int indentLevel;       // 缩进级别
        int xPosition;         // X轴位置
    };

    void drawBracketGuides(const QRect &visibleRect, int contentYOffset, int contentXOffset);
    QVector<BracketPair> findBracketPairs(int firstVisibleBlock, int lastVisibleBlock);
    int getIndentLevel(const QString &text, int bracketPos);
    bool isInStringOrComment(const QTextBlock &block, int position);
    bool hasCharacterAtPosition(const QTextBlock &block, int xPosition);

    int m_currentNumberAreaWidth = 0;
    bool m_enableBracketGuides = true;  // 是否启用花括号连接线
};
}   // namespace uos_ai

#endif // LINENUMBERTEXTEDIT_H
