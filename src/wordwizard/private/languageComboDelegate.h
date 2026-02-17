#ifndef LANGUAGECOMBODELEGATE
#define LANGUAGECOMBODELEGATE

#include "uosai_global.h"

#include <QStyledItemDelegate>
#include <QObject>

namespace uos_ai {
// Set custom item delegate to show items with smaller font
class LanguageComboDelegate : public QStyledItemDelegate {
    Q_OBJECT

private:
    mutable QFont m_index1Font; // 为了处理藏语格式，保留前一位item的小字字体，是藏语item的小字部分保持中英格式
    mutable int m_index1SmallHeight;
    int m_currentIndex = -1;

public:
    explicit LanguageComboDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    
    void setCurrentIndex(int index) { m_currentIndex = index; }
};

}

#endif // LANGUAGECOMBODELEGATE
