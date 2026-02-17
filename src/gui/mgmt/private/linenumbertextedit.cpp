#include "linenumbertextedit.h"

#include <QPainter>
#include <QTextBlock>
#include <QTextLayout>
#include <QScrollBar>
#include <QMimeData>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

LineNumberTextEdit::LineNumberTextEdit(QWidget *parent)
    : DTextEdit(parent)
{
    // 连接信号
    connect(this, &DTextEdit::textChanged, this, &LineNumberTextEdit::updateLineNumberAreaWidth);
    connect(document(), &QTextDocument::contentsChange, this, [this](int, int, int) {
        updateLineNumberAreaWidth();
    });
    
    updateLineNumberAreaWidth();
}

void LineNumberTextEdit::resizeEvent(QResizeEvent *e)
{
    DTextEdit::resizeEvent(e);
    
    updateLineNumberAreaWidth();
}

void LineNumberTextEdit::paintEvent(QPaintEvent *event)
{
    setViewportMargins(3, 3, 3, 3);
    QTextDocument *doc = document();
    QTextFrame *rootFrame = doc->rootFrame();
    QTextFrameFormat format = rootFrame->frameFormat();
    format.setLeftMargin(m_currentNumberAreaWidth + 12);
    rootFrame->setFrameFormat(format);

    // 获取第一个可见块
    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::Start);

    // 计算可见区域
    QRect visibleRect = viewport()->rect();
    int contentYOffset = verticalScrollBar()->value();
    
    // 计算行高
    int lineHeight = fontMetrics().height();
    if (document()->firstBlock().isValid()) {
         QTextLayout *layout = document()->firstBlock().layout();
        if (layout && layout->lineCount() > 0) {
            // 使用第一行的高度而不是整个文本块的高度
            int firstLineHeight = layout->lineAt(0).height();
            if (firstLineHeight > 0)
                lineHeight = firstLineHeight;
        }
    }

    int topMargin = format.topMargin() + 1;

    // 计算可见行范围
    int firstVisibleLine = contentYOffset / lineHeight;
    int lastVisibleLine = firstVisibleLine + visibleRect.height() / lineHeight + 1;
    
    // 绘制行号
    QPainter painter(viewport());
    painter.setPen(Qt::darkGray);
    
    for (int line = firstVisibleLine; line <= lastVisibleLine; ++line) {
        // 计算行号位置
        int yPos = line * lineHeight - contentYOffset;
        
        // 确保行号在可见区域内
        if (yPos + lineHeight >= 0 && yPos <= visibleRect.height()) {
            QString number = QString::number(line + 1);
            painter.drawText(0, yPos + topMargin, m_currentNumberAreaWidth, lineHeight,
                            Qt::AlignRight, number);
        }
    }

    DTextEdit::paintEvent(event);
}

bool LineNumberTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    // 允许纯文本粘贴
    return source->hasText();
}

void LineNumberTextEdit::insertFromMimeData(const QMimeData *source)
{
    if (source->hasText()) {
        // 获取纯文本内容，去掉所有格式
        QString text = source->text();
        
        // 插入纯文本
        QTextCursor cursor = textCursor();
        cursor.insertText(text);
    }
}

int LineNumberTextEdit::lineNumberAreaWidth()
{
    // 计算需要的数字位数
    int digits = 1;
    int lineCount = document()->blockCount();
    int max = qMax(1, lineCount);
    
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    
    // 计算宽度：边距 + 数字宽度
    int space = 5 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void LineNumberTextEdit::updateLineNumberAreaWidth()
{
    int numberAreaWidth = lineNumberAreaWidth();
    if (m_currentNumberAreaWidth == numberAreaWidth)
        return;

    m_currentNumberAreaWidth = numberAreaWidth;
}
