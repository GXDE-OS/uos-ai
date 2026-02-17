#ifndef SKILLLISTWIDGET_H
#define SKILLLISTWIDGET_H

#include <DWidget>
#include <DCommandLinkButton>
#include <DBackgroundGroup>

#include <QList>
#include <QString>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QTimer>
#include <QQueue>

class ThemedLable;

namespace uos_ai {

class CustomFunction;
class SkillListItem;
class AddSkillDialog;

class SkillListWidget : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT
public:
    explicit SkillListWidget(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    void addSkill(const CustomFunction &skill);
    void removeSkill(const QString &name);

signals:
    void skillListChanged();
    void skillAddedSuccessfully(const QString &message);

private slots:
    void onAddButtonClicked();
    void onDeleteSkill(const QString &name);
    void onEditSkill(const QString &name);
    void onHideSkill(const QString &name);
    void onThemeTypeChanged();
    void onAnimationFinished();
    void onDragStarted(SkillListItem *item, const QPoint &startPos);
    void onDragMoved(const QPoint &currentPos);
    void onDragFinished();

private:
    void initUI();
    void initConnect();
    void refreshSkillList();

    SkillListItem* createSkillItem(const CustomFunction &skill);

    void updateItemPositions(bool animated = true);
    void animateItemToPosition(SkillListItem *item, int targetIndex);
    int calculateDropIndex(const QPoint &pos);
    QRect getItemRect(int index) const;
    void animateSkillToEnd(SkillListItem *item, int fromIndex);
    void onHideAnimationFinished();
    void processNextHideOperation();
    int getEnabledSkillCount() const;
    void animateSkillToEnabledEnd(SkillListItem *item);
    void sortSkillsByEnabledState();
    int calculateTotalHeight() const;

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    ThemedLable *m_pWidgetLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_pHasSkillWidget = nullptr;
    DTK_WIDGET_NAMESPACE::DCommandLinkButton *m_pAddButton = nullptr;
    AddSkillDialog *m_addSkillDialog = nullptr;

    QList<SkillListItem*> m_skillItems;

    // 拖拽相关
    SkillListItem *m_draggedItem = nullptr;
    int m_draggedIndex = -1;
    int m_dropIndex = -1;
    QPoint m_dragStartPos;
    QPoint m_dragOffset;
    bool m_isDragging = false;

    // 动画相关
    QParallelAnimationGroup *m_animationGroup = nullptr;
    QTimer *m_layoutTimer = nullptr;  // 用于延迟布局更新

    // 隐藏动画状态管理
    bool m_isHideAnimating = false;
    QQueue<QPair<QString, int>> m_pendingHideOperations;

    static const int ITEM_WIDTH = 560;
    static const int ITEM_HEIGHT = 40;
    static const int ITEM_SPACING = 6;
    static const int ANIMATION_DURATION = 200;
};
}

#endif // SKILLLISTWIDGET_H
