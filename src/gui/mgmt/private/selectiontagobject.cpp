#include "selectiontagobject.h"

#include <QFontMetrics>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextFragment>
#include <DFontSizeManager>

#include <functional>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;


SelectionTagObject::SelectionTagObject(QObject *parent)
    : QObject(parent)
{
}

QSizeF SelectionTagObject::intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(doc)
    Q_UNUSED(posInDocument)
    
    QString tagText = getTagText(format);
    if (tagText.isEmpty()) {
        tagText = getDefaultTagText();
    }

    QFont font = doc->defaultFont();
    if (format.hasProperty(QTextFormat::FontFamily)) {
        font.setFamily(format.stringProperty(QTextFormat::FontFamily));
    }
    if (format.hasProperty(QTextFormat::FontPointSize)) {
        font.setPointSizeF(format.doubleProperty(QTextFormat::FontPointSize));
    }

    font = getTagFont(font);
    
    return calculateTagSize(font, tagText);
}

void SelectionTagObject::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, 
                                   int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(doc)
    Q_UNUSED(posInDocument)
    
    QString tagText = getTagText(format);
    if (tagText.isEmpty()) {
        tagText = getDefaultTagText();
    }
    
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    drawTagBackground(painter, rect);

    QFont font = painter->font();
    if (format.hasProperty(QTextFormat::FontFamily)) {
        font.setFamily(format.stringProperty(QTextFormat::FontFamily));
    }
    if (format.hasProperty(QTextFormat::FontPointSize)) {
        font.setPointSizeF(format.doubleProperty(QTextFormat::FontPointSize));
    }

    font = getTagFont(font);
    drawTagText(painter, rect, tagText, font);
    painter->restore();
}

QTextCharFormat SelectionTagObject::createSelectionTagFormat(const QString &tagText)
{
    QTextCharFormat format;
    format.setObjectType(SelectionTagType);
    
    QString actualTagText = tagText.isEmpty() ? getDefaultTagText() : tagText;
    format.setProperty(TagTextProperty, actualTagText);

    format.setVerticalAlignment(QTextCharFormat::AlignBaseline);
    
    return format;
}

QString SelectionTagObject::getTagText(const QTextFormat &format)
{
    QString text = format.property(TagTextProperty).toString();
    return text.isEmpty() ? getDefaultTagText() : text;
}

bool SelectionTagObject::isSelectionTagFormat(const QTextFormat &format)
{
    if (format.objectType() != SelectionTagType) {
        return false;
    }

    return format.hasProperty(TagTextProperty);
}

void SelectionTagObject::insertSelectionTag(QTextCursor &cursor, const QString &tagText)
{
    QTextCharFormat tagFormat = createSelectionTagFormat(tagText);
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), tagFormat);
}

bool SelectionTagObject::hasSelectionTag(QTextDocument *document)
{
    return findSelectionTagPosition(document).first != -1;
}

QPair<int, int> SelectionTagObject::findSelectionTagPosition(QTextDocument *document)
{
    QPair<int, int> result(-1, -1);
    
    visitAllFragments(document, [&result](QTextFragment &fragment, int start, int end) -> bool {
        QTextCharFormat format = fragment.charFormat();
        QString text = fragment.text();

        bool isTag = isSelectionTagFormat(format) && text.contains(QChar::ObjectReplacementCharacter);
        
        if (isTag) {
            result = qMakePair(start, end);
            return false; // 找到第一个就停止
        }
        return true; // 继续搜索
    });
    
    return result;
}

void SelectionTagObject::removeAllSelectionTags(QTextDocument *document)
{
    QTextCursor cursor(document);

    QList<QPair<int, int>> tagPositions;
    visitAllFragments(document, [&tagPositions](QTextFragment &fragment, int start, int end) -> bool {
        QTextCharFormat format = fragment.charFormat();
        QString text = fragment.text();

        bool isTag = isSelectionTagFormat(format) && text.contains(QChar::ObjectReplacementCharacter);
        
        if (isTag) {
            tagPositions.append(qMakePair(start, end));
        }
        return true;
    });

    std::sort(tagPositions.begin(), tagPositions.end(), 
              [](const QPair<int, int> &a, const QPair<int, int> &b) {
                  return a.first > b.first;
              });
    
    for (const auto &pos : tagPositions) {
        cursor.setPosition(pos.first);
        cursor.setPosition(pos.second, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
}

bool SelectionTagObject::isPositionInSelectionTag(QTextDocument *document, int position, 
                                                 int *tagStart, int *tagEnd)
{
    bool found = false;
    
    visitAllFragments(document, [&](QTextFragment &fragment, int start, int end) -> bool {
        QTextCharFormat format = fragment.charFormat();
        QString text = fragment.text();

        bool isTag = isSelectionTagFormat(format) && text.contains(QChar::ObjectReplacementCharacter);
        
        if (isTag && position >= start && position <= end) {
            found = true;
            if (tagStart) *tagStart = start;
            if (tagEnd) *tagEnd = end;
            return false; // 找到就停止
        }
        return true;
    });
    
    return found;
}

QString SelectionTagObject::convertToSaveFormat(const QString &displayText)
{
    QString result = displayText;

    QChar objectChar = QChar::ObjectReplacementCharacter;
    QString saveTag = "${SELECTION}";
    
    result.replace(objectChar, saveTag);
    
    return result;
}

QString SelectionTagObject::convertFromSaveFormat(const QString &savedText)
{
    QString result = savedText;

    QString saveTag = "${SELECTION}";
    QString displayTag = getDefaultTagText();
    
    result.replace(saveTag, displayTag);
    
    return result;
}

QString SelectionTagObject::extractSaveFormatFromDocument(QTextDocument *document)
{
    QString result;
    
    QTextBlock block = document->firstBlock();
    while (block.isValid()) {
        QTextBlock::iterator it;
        for (it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            
            if (isSelectionTagFormat(fragment.charFormat())) {
                result += "${SELECTION}";
            } else {
                result += fragment.text();
            }
        }
        
        block = block.next();
        if (block.isValid()) {
            result += "\n"; // 添加换行符
        }
    }
    
    return result;
}

QString SelectionTagObject::getDefaultTagText()
{
    return QObject::tr("{selection}");
}

QSizeF SelectionTagObject::calculateTagSize(const QFont &font, const QString &tagText) const
{
    QFontMetrics fm(font);

    int textWidth = fm.horizontalAdvance(tagText);
    int textHeight = fm.height();
    int width = textWidth + 14;
    int height = textHeight + 2;
    
    return QSizeF(width, height);
}

void SelectionTagObject::drawTagBackground(QPainter *painter, const QRectF &rect) const
{
    QColor bgColor = getTagBackgroundColor();
    
    painter->setBrush(bgColor);
    painter->setPen(Qt::NoPen);

    painter->drawRoundedRect(rect.adjusted(3, 1, -3, 1), 4, 4);
}

void SelectionTagObject::drawTagText(QPainter *painter, const QRectF &rect, const QString &text, const QFont &font) const
{
    painter->setFont(font);
    painter->setPen(getTagTextColor());

    painter->drawText(rect, Qt::AlignCenter, text);
}

QColor SelectionTagObject::getTagBackgroundColor() const
{
    DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
    QColor bgColor = palette.color(DPalette::Highlight);
    bgColor.setAlphaF(0.2); // 背景色0.2%
    return bgColor;
}

QColor SelectionTagObject::getTagTextColor() const
{
    DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
    return palette.color(DPalette::Highlight);
}

QFont SelectionTagObject::getTagFont(const QFont &baseFont) const
{
    QFont tagFont = DFontSizeManager::instance()->get(DFontSizeManager::T6, QFont::Normal, baseFont);
    return tagFont;
}

QTextCharFormat SelectionTagObject::findSelectionTagFormat(QTextDocument *document)
{
    QTextCharFormat result;
    
    visitAllFragments(document, [&result](QTextFragment &fragment, int start, int end) -> bool {
        Q_UNUSED(start)
        Q_UNUSED(end)
        if (isSelectionTagFormat(fragment.charFormat())) {
            result = fragment.charFormat();
            return false; // 找到就停止
        }
        return true;
    });
    
    return result;
}

void SelectionTagObject::visitAllFragments(QTextDocument *document, 
                                         std::function<bool(QTextFragment&, int, int)> visitor)
{
    QTextBlock block = document->firstBlock();
    while (block.isValid()) {
        QTextBlock::iterator it;
        for (it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            int start = fragment.position();
            int end = start + fragment.length();
            
            if (!visitor(fragment, start, end)) {
                return; // 访问者要求停止
            }
        }
        block = block.next();
    }
} 
