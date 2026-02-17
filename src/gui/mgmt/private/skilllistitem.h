#ifndef SKILLLISTITEM_H
#define SKILLLISTITEM_H

#include <DWidget>
#include <QString>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QRect>

namespace uos_ai {
class SkillListItem : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT
public:
    explicit SkillListItem(const QString &name, const QString &iconName, DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    void setCustomSkill(bool isCustom) { m_isCustomSkill = isCustom; update(); }
    bool isCustomSkill() const { return m_isCustomSkill; }
    QString getName() const { return m_name; }
    bool isSkillVisible() const { return m_skillVisible; }
    void setSkillVisible(bool visible) { m_skillVisible = visible; update(); }
    void setWindowActive(bool active) { m_windowActive = active; update(); }
    bool isWindowActive() const { return m_windowActive; }

    bool shouldShowGrayed() const { return !m_skillVisible || !m_windowActive; }

signals:
    void signalDeleteItem(const QString &name);
    void signalEditItem(const QString &name);
    void signalHideItem(const QString &name);
    void signalDragStarted(SkillListItem *item, const QPoint &globalPos);
    void signalDragMoved(const QPoint &globalPos);
    void signalDragFinished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
#ifdef COMPILE_ON_QT6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif

private:
    void initUI();
    QRect getDragHandleRect() const;
    QRect getIconRect() const;
    QRect getNameRect() const;
    QRect getHideButtonRect() const;
    QRect getEditButtonRect() const;
    QRect getDeleteButtonRect() const;
    void drawDragHandle(QPainter &painter, const QRect &rect);
    void drawIcon(QPainter &painter, const QRect &rect);
    void drawName(QPainter &painter, const QRect &rect);
    void drawHideButton(QPainter &painter, const QRect &rect);
    void drawEditButton(QPainter &painter, const QRect &rect);
    void drawDeleteButton(QPainter &painter, const QRect &rect);

private:
    QString m_name;
    QString m_iconName;
    bool m_isDragging = false;
    bool m_isHovered = false;
    bool m_hideButtonHovered = false;
    bool m_editButtonHovered = false;
    bool m_deleteButtonHovered = false;
    bool m_skillVisible = true;
    bool m_isCustomSkill = false;
    bool m_windowActive = true;
    QPoint m_dragStartPosition;

    static const int DRAG_HANDLE_WIDTH = 8;
    static const int ICON_SIZE = 25;
    static const int HIDE_BUTTON_SIZE = 22;
    static const int EDIT_BUTTON_SIZE = 16;
    static const int DELETE_BUTTON_SIZE = 16;
    static const int MARGIN = 10;
    static const int SPACING = 6;
};
}

#endif // SKILLLISTITEM_H
