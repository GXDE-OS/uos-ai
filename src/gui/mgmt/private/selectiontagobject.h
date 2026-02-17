#ifndef SELECTIONTAGOBJECT_H
#define SELECTIONTAGOBJECT_H
#include "uosai_global.h"

#include <DGuiApplicationHelper>
#include <DPalette>

#include <QObject>
#include <QTextObjectInterface>
#include <QTextFormat>
#include <QSizeF>
#include <QPainter>
#include <QRectF>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QTextFragment>

namespace uos_ai {

// 自定义选择标签对象类型ID
enum SelectionTagObjectType {
    SelectionTagType = QTextFormat::UserObject + 100  // 避免与其他对象冲突
};

// 选择标签对象属性
enum SelectionTagProperty {
    TagTextProperty = QTextFormat::UserProperty + 100,
    TagIdProperty = QTextFormat::UserProperty + 101
};

class SelectionTagObject : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit SelectionTagObject(QObject *parent = nullptr);

    QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format) override;
    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, 
                    int posInDocument, const QTextFormat &format) override;

    static QTextCharFormat createSelectionTagFormat(const QString &tagText = QString());
    static QString getTagText(const QTextFormat &format);
    static bool isSelectionTagFormat(const QTextFormat &format);

    static void insertSelectionTag(QTextCursor &cursor, const QString &tagText = QString());
    static bool hasSelectionTag(QTextDocument *document);
    static QPair<int, int> findSelectionTagPosition(QTextDocument *document);
    static void removeAllSelectionTags(QTextDocument *document);

    static bool isPositionInSelectionTag(QTextDocument *document, int position, 
                                       int *tagStart = nullptr, int *tagEnd = nullptr);

    static QString convertToSaveFormat(const QString &displayText);
    static QString convertFromSaveFormat(const QString &savedText);

    static QString extractSaveFormatFromDocument(QTextDocument *document);

    static QString getDefaultTagText();

signals:
    void tagClicked(const QString &tagText);

private:
    QSizeF calculateTagSize(const QFont &font, const QString &tagText) const;
    void drawTagBackground(QPainter *painter, const QRectF &rect) const;
    void drawTagText(QPainter *painter, const QRectF &rect, const QString &text, const QFont &font) const;

    QColor getTagBackgroundColor() const;
    QColor getTagTextColor() const;
    QFont getTagFont(const QFont &baseFont) const;

    static QTextCharFormat findSelectionTagFormat(QTextDocument *document);
    static void visitAllFragments(QTextDocument *document, 
                                std::function<bool(QTextFragment&, int, int)> visitor);
};
}

#endif // SELECTIONTAGOBJECT_H 
