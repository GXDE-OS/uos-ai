#include "linenumbertextedit.h"

#include <QPainter>
#include <QTextBlock>
#include <QTextLayout>
#include <QScrollBar>
#include <QMimeData>
#include <QStack>
#include <QAbstractTextDocumentLayout>
#include <DGuiApplicationHelper>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
using namespace uos_ai;

LineNumberTextEdit::LineNumberTextEdit(QWidget *parent)
    : DTextEdit(parent)
{
    // 禁用自动换行，避免行号和竖线绘制异常
    setLineWrapMode(QTextEdit::NoWrap);

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
    int contentXOffset = horizontalScrollBar()->value();
    
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

    DTextEdit::paintEvent(event);

    // 绘制行号区域背景，防止横向滚动时文本内容与行号重叠
    QPainter painter(viewport());

    // 根据主题模式设置背景色
    QColor backgroundColor;
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
        backgroundColor = QColor(68, 68, 68);  // 深色模式
    } else {
        backgroundColor = QColor(218, 218, 218);  // 浅色模式
    }

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(backgroundColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, m_currentNumberAreaWidth, visibleRect.height(), 2, 2);

    // 绘制行号
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

    // 绘制花括号连接线
    if (m_enableBracketGuides) {
        drawBracketGuides(visibleRect, contentYOffset, contentXOffset);
    }
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

void LineNumberTextEdit::drawBracketGuides(const QRect &visibleRect, int contentYOffset, int contentXOffset)
{
    QAbstractTextDocumentLayout *layout = document()->documentLayout();
    if (!layout)
        return;

    // 计算可见块范围
    QTextBlock firstBlock = document()->firstBlock();
    int blockNumber = 0;

    // 找到第一个可见块
    while (firstBlock.isValid()) {
        QRectF blockRect = layout->blockBoundingRect(firstBlock);
        if (blockRect.bottom() >= contentYOffset) {
            break;
        }
        firstBlock = firstBlock.next();
        blockNumber++;
    }

    int firstVisibleBlock = blockNumber;

    // 找到最后一个可见块
    QTextBlock lastBlock = firstBlock;
    int lastVisibleBlock = firstVisibleBlock;
    while (lastBlock.isValid()) {
        QRectF blockRect = layout->blockBoundingRect(lastBlock);
        if (blockRect.top() > contentYOffset + visibleRect.height()) {
            break;
        }
        lastBlock = lastBlock.next();
        lastVisibleBlock++;
    }

    // 查找花括号对
    QVector<BracketPair> pairs = findBracketPairs(firstVisibleBlock - 50, lastVisibleBlock + 50);

    // 设置绘制样式
    QPainter painter(viewport());
    painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    painter.setPen(Qt::darkGray);

    // 绘制连接线
    for (const BracketPair &pair : pairs) {
        QTextBlock openBlock = document()->findBlockByNumber(pair.openBlockNumber);
        QTextBlock closeBlock = document()->findBlockByNumber(pair.closeBlockNumber);

        if (!openBlock.isValid() || !closeBlock.isValid())
            continue;

        QRectF openRect = layout->blockBoundingRect(openBlock);
        QRectF closeRect = layout->blockBoundingRect(closeBlock);

        // 只绘制至少部分在可见区域内的线
        if (openRect.bottom() < contentYOffset - 100 && closeRect.top() > contentYOffset + visibleRect.height() + 100)
            continue;

        int x = pair.xPosition;
        int y1 = openRect.bottom() - contentYOffset;
        int y2 = closeRect.top() - contentYOffset;

        // 考虑横向滚动偏移
        int xDisplay = x - contentXOffset;

        // 如果竖线在可见区域外，跳过
        if (xDisplay < 0 || xDisplay > visibleRect.width())
            continue;

        // 分段绘制竖线，避开有字符的位置
        if (y2 > y1 + 2) {  // 至少有2像素高度才绘制
            int segmentStart = y1;

            // 遍历竖线经过的所有块
            for (int blockNum = pair.openBlockNumber + 1; blockNum < pair.closeBlockNumber; blockNum++) {
                QTextBlock currentBlock = document()->findBlockByNumber(blockNum);
                if (!currentBlock.isValid())
                    continue;

                QRectF blockRect = layout->blockBoundingRect(currentBlock);
                int blockTop = blockRect.top() - contentYOffset;
                int blockBottom = blockRect.bottom() - contentYOffset;

                // 检查该块在 x 位置是否有字符
                if (hasCharacterAtPosition(currentBlock, x)) {
                    // 如果有字符，先绘制到这个块之前的线段
                    if (segmentStart < blockTop) {
                        painter.drawLine(xDisplay, segmentStart, xDisplay, blockTop);
                    }
                    // 跳过这个块，从块底部继续
                    segmentStart = blockBottom;
                }
            }

            // 绘制最后一段
            if (segmentStart < y2) {
                painter.drawLine(xDisplay, segmentStart, xDisplay, y2);
            }
        }
    }
}

QVector<LineNumberTextEdit::BracketPair> LineNumberTextEdit::findBracketPairs(int firstBlock, int lastBlock)
{
    QVector<BracketPair> pairs;
    QStack<BracketPair> stack;

    // 确保范围有效
    firstBlock = qMax(0, firstBlock);
    lastBlock = qMin(document()->blockCount() - 1, lastBlock);

    QTextBlock block = document()->findBlockByNumber(firstBlock);

    for (int blockNum = firstBlock; blockNum <= lastBlock && block.isValid(); blockNum++, block = block.next()) {
        QString text = block.text();

        for (int i = 0; i < text.length(); i++) {
            QChar c = text[i];

            // 跳过字符串和注释中的括号
            if (isInStringOrComment(block, i))
                continue;

            if (c == '{') {
                BracketPair pair;
                pair.openBlockNumber = blockNum;
                pair.indentLevel = getIndentLevel(text, i);
                pair.xPosition = m_currentNumberAreaWidth + 12 + pair.indentLevel * fontMetrics().horizontalAdvance(' ');
                stack.push(pair);
            } else if (c == '}' && !stack.isEmpty()) {
                BracketPair pair = stack.pop();
                pair.closeBlockNumber = blockNum;
                pairs.append(pair);
            }
        }
    }

    return pairs;
}

int LineNumberTextEdit::getIndentLevel(const QString &text, int bracketPos)
{
    int indent = 0;
    for (int i = 0; i < bracketPos && i < text.length(); i++) {
        if (text[i] == ' ') {
            indent++;
        } else if (text[i] == '\t') {
            indent += 4;  // 制表符算4个空格
        } else if (!text[i].isSpace()) {
            // 如果括号前有非空白字符，使用该位置
            indent = i;
            break;
        }
    }
    return indent;
}

bool LineNumberTextEdit::isInStringOrComment(const QTextBlock &block, int position)
{
    // 简单实现：检查是否在引号或注释中
    QString text = block.text();
    if (position >= text.length())
        return false;

    bool inString = false;
    bool inSingleQuote = false;
    QChar escapeChar = '\\';
    bool escaped = false;

    // 检查是否在字符串中
    for (int i = 0; i < position; i++) {
        QChar c = text[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (c == escapeChar) {
            escaped = true;
            continue;
        }

        if (c == '"' && !inSingleQuote) {
            inString = !inString;
        } else if (c == '\'' && !inString) {
            inSingleQuote = !inSingleQuote;
        }
    }

    if (inString || inSingleQuote)
        return true;

    // 检查是否在单行注释中 (//)
    int commentPos = text.indexOf("//");
    if (commentPos >= 0 && commentPos < position)
        return true;

    // TODO: 可以添加多行注释 /* */ 的检测

    return false;
}

bool LineNumberTextEdit::hasCharacterAtPosition(const QTextBlock &block, int xPosition)
{
    if (!block.isValid())
        return false;

    QString text = block.text();
    if (text.isEmpty())
        return false;

    // 计算文档左边距
    QTextDocument *doc = document();
    QTextFrame *rootFrame = doc->rootFrame();
    QTextFrameFormat format = rootFrame->frameFormat();
    int leftMargin = format.leftMargin();

    // 遍历文本中的每个字符，计算其 x 位置
    int currentX = leftMargin;
    QFontMetrics fm = fontMetrics();

    for (int i = 0; i < text.length(); i++) {
        QChar c = text[i];
        int charWidth = fm.horizontalAdvance(c);

        // 检查竖线位置是否在当前字符的范围内
        if (xPosition >= currentX && xPosition < currentX + charWidth) {
            // 检查是否为非空字符
            if (!c.isSpace()) {
                return true;
            }
        }

        currentX += charWidth;
    }

    return false;
}
