#include "skilllistwidget.h"
#include "skilllistitem.h"
#include "themedlable.h"
#include "addskilldialog.h"
#include "wordwizard/wordwizard.h"
#include "dconfigmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <DLabel>

#include <DBackgroundGroup>
#include <DGuiApplicationHelper>
#include <DFontSizeManager>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QMimeData>
#include <QDebug>
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QResizeEvent>
#include <QLoggingCategory>
#include <algorithm>

using namespace uos_ai;
DWIDGET_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

SkillListWidget::SkillListWidget(DWidget *parent)
    : DWidget(parent)
    , m_draggedItem(nullptr)
    , m_draggedIndex(-1)
    , m_dropIndex(-1)
    , m_isDragging(false)
    , m_animationGroup(nullptr)
    , m_layoutTimer(new QTimer(this))
    , m_isHideAnimating(false)
{
    m_layoutTimer->setSingleShot(true);
    m_layoutTimer->setInterval(50);

    initUI();
    initConnect();
    onThemeTypeChanged();
    refreshSkillList();
}

void SkillListWidget::initUI()
{
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);

    m_pWidgetLabel = new ThemedLable(tr("Skill Management"), this);
    m_pWidgetLabel->setPaletteColor(QPalette::WindowText, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_pWidgetLabel, DFontSizeManager::T6, QFont::Medium);

    m_pAddButton = new DCommandLinkButton(tr("Add Skill"), this);
    DFontSizeManager::instance()->bind(m_pAddButton, DFontSizeManager::T8, QFont::Normal);

    titleLayout->addWidget(m_pWidgetLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(m_pAddButton);

    DLabel *descLabel = new DLabel(tr("The first 4 skills will be displayed on the toolbar, and others will be hidden in the More menu."));
    descLabel->setWordWrap(true);
    descLabel->setFixedWidth(560);
    DFontSizeManager::instance()->bind(descLabel, DFontSizeManager::T8, QFont::Normal);
    descLabel->setForegroundRole(DPalette::TextTips);

    m_pHasSkillWidget = new DWidget(this);
    m_pHasSkillWidget->setContentsMargins(0, 0, 0, 0);
    m_pHasSkillWidget->setMinimumHeight(100);
    m_pHasSkillWidget->setAutoFillBackground(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addLayout(titleLayout);
    layout->addSpacing(6);
    layout->addWidget(descLabel, 0, Qt::AlignLeft);
    layout->addWidget(m_pHasSkillWidget);

    setLayout(layout);
}

void SkillListWidget::initConnect()
{
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &SkillListWidget::onThemeTypeChanged);
    connect(m_pAddButton, &DCommandLinkButton::clicked, this, &SkillListWidget::onAddButtonClicked);

    connect(m_layoutTimer, &QTimer::timeout, this, [this]() {
        updateItemPositions(false);
    });
}

void SkillListWidget::onThemeTypeChanged()
{
    if (m_pHasSkillWidget) {
        QPalette pl = m_pHasSkillWidget->palette();
        pl.setColor(QPalette::Window, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        m_pHasSkillWidget->setPalette(pl);
    }
}

void SkillListWidget::addSkill(const CustomFunction &skill)
{
    int enabledCount = getEnabledSkillCount();

    WordWizard::kCustomFunctionList.insert(enabledCount, skill);

    SkillListItem *item = createSkillItem(skill);
    if (item) {
        m_skillItems.insert(enabledCount, item);
        item->show();

        updateItemPositions(true);

        int totalHeight = calculateTotalHeight();
        m_pHasSkillWidget->setFixedHeight(totalHeight);
    } else {
        qCWarning(logAIGUI) << "Failed to create skill item for skill:" << skill.name;
    }

    WordWizard::saveCustomFunctions();
    emit skillListChanged();
}

SkillListItem* SkillListWidget::createSkillItem(const CustomFunction &skill)
{
    QString displayName;
    QString iconName;
    
    if (skill.isCustom) {
        // 自定义技能
        displayName = skill.name;
        iconName = "uos-ai-assistant_custom";
    } else {
        // 默认技能
        displayName = WordWizard::getDefaultSkillName(skill.defaultFunctionType);
        iconName = WordWizard::getDefaultSkillIcon(skill.defaultFunctionType).name();
    }

    SkillListItem *item = new SkillListItem(displayName, iconName, m_pHasSkillWidget);
    item->setCustomSkill(skill.isCustom);
    item->setSkillVisible(!skill.isHidden);

    if (item) {
        connect(item, &SkillListItem::signalDeleteItem, this, &SkillListWidget::onDeleteSkill);
        connect(item, &SkillListItem::signalEditItem, this, &SkillListWidget::onEditSkill);
        connect(item, &SkillListItem::signalHideItem, this, &SkillListWidget::onHideSkill);
        connect(item, &SkillListItem::signalDragStarted, this, &SkillListWidget::onDragStarted);
        connect(item, &SkillListItem::signalDragMoved, this, &SkillListWidget::onDragMoved);
        connect(item, &SkillListItem::signalDragFinished, this, &SkillListWidget::onDragFinished);
    }

    return item;
}

void SkillListWidget::removeSkill(const QString &name)
{
    for (int i = 0; i < WordWizard::kCustomFunctionList.size(); ++i) {
        const CustomFunction &skill = WordWizard::kCustomFunctionList[i];
        QString skillDisplayName = skill.isCustom ? skill.name : WordWizard::getDefaultSkillName(skill.defaultFunctionType);
        
        if (skillDisplayName == name) {
            WordWizard::kCustomFunctionList.removeAt(i);
            break;
        }
    }

    for (int i = 0; i < m_skillItems.size(); ++i) {
        if (m_skillItems[i]->getName() == name) {
            SkillListItem *itemToDelete = m_skillItems.takeAt(i);
            itemToDelete->deleteLater();
            break;
        }
    }

    updateItemPositions(true);

    int totalHeight = calculateTotalHeight();
    m_pHasSkillWidget->setFixedHeight(totalHeight);

    WordWizard::saveCustomFunctions();
    emit skillListChanged();
}

void SkillListWidget::onAddButtonClicked()
{
    if (!DConfigManager::checkConfigAvailable(WORDWIZARD_GROUP, WORDWIZARD_CUSTOM_FUNCTIONS))
        return; //dconfig加载失败

    m_addSkillDialog = new AddSkillDialog();
    m_addSkillDialog->resetDialog();
    if (m_addSkillDialog->exec() == QDialog::Accepted) {
        QString skillName = m_addSkillDialog->getSkillName();
        QString skillCommand = m_addSkillDialog->getSkillCommand();

        CustomFunction newSkill(skillName, skillCommand, true);

        addSkill(newSkill);

        emit skillAddedSuccessfully(tr("Saved"));
    }
    m_addSkillDialog->deleteLater();
}

void SkillListWidget::onDeleteSkill(const QString &name)
{
    bool isCustomSkill = false;
    for (const auto &skill : WordWizard::kCustomFunctionList) {
        QString skillDisplayName = skill.isCustom ? skill.name : WordWizard::getDefaultSkillName(skill.defaultFunctionType);
        if (skillDisplayName == name && skill.isCustom) {
            isCustomSkill = true;
            break;
        }
    }

    if (!isCustomSkill) {
        qCWarning(logAIGUI) << "Cannot delete default skill:" << name;
        return;
    }

    DDialog dlg(this);
    dlg.setFixedWidth(380);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.setMessage(tr("Are you sure you want to delete the skill \"%1\"?").arg(name));
    dlg.addButton(tr("Cancel"), true, DDialog::ButtonNormal);
    dlg.addButton(tr("Delete"), true, DDialog::ButtonWarning);
    if (dlg.exec() == DDialog::Accepted)
        removeSkill(name);
}

void SkillListWidget::onEditSkill(const QString &name)
{
    int skillIndex = -1;
    
    for (int i = 0; i < WordWizard::kCustomFunctionList.size(); ++i) {
        const CustomFunction &skill = WordWizard::kCustomFunctionList[i];
        QString skillDisplayName = skill.isCustom ? skill.name : WordWizard::getDefaultSkillName(skill.defaultFunctionType);
        
        if (skillDisplayName == name) {
            skillIndex = i;
            break;
        }
    }
    
    if (skillIndex == -1) {
        qCWarning(logAIGUI) << "Skill not found for editing:" << name;
        return;
    }

    if (!WordWizard::kCustomFunctionList[skillIndex].isCustom) {
        qCWarning(logAIGUI) << "Cannot edit default skill:" << name;
        return;
    }

    AddSkillDialog *editDialog = new AddSkillDialog();
    const CustomFunction &currentSkill = WordWizard::kCustomFunctionList[skillIndex];
    editDialog->setSkillData(currentSkill.name, currentSkill.prompt);
    
    if (editDialog->exec() == QDialog::Accepted) {
        QString newSkillName = editDialog->getSkillName();
        QString newSkillCommand = editDialog->getSkillCommand();

        CustomFunction updatedSkill(newSkillName, newSkillCommand, true);
        updatedSkill.isHidden = currentSkill.isHidden;

        WordWizard::kCustomFunctionList[skillIndex] = updatedSkill;

        refreshSkillList();

        WordWizard::saveCustomFunctions();
        emit skillListChanged();
    } else {
        qCDebug(logAIGUI) << "Skill editing cancelled:" << name;
    }
    
    editDialog->deleteLater();
}

void SkillListWidget::onHideSkill(const QString &name)
{
    if (m_isDragging) {
        qCWarning(logAIGUI) << "Cannot hide/show skill while dragging:" << name;
        return;
    }

    SkillListItem *item = nullptr;
    int currentIndex = -1;
    for (int i = 0; i < m_skillItems.size(); ++i) {
        if (m_skillItems[i]->getName() == name) {
            item = m_skillItems[i];
            currentIndex = i;
            break;
        }
    }

    if (!item) {
        qCWarning(logAIGUI) << "Skill item not found for hide/show operation:" << name;
        return;
    }

    bool wasVisible = item->isSkillVisible();

    int skillIndex = -1;
    for (int i = 0; i < WordWizard::kCustomFunctionList.size(); ++i) {
        CustomFunction &skill = WordWizard::kCustomFunctionList[i];
        QString skillDisplayName = skill.isCustom ? skill.name : WordWizard::getDefaultSkillName(skill.defaultFunctionType);
        
        if (skillDisplayName == name) {
            skill.isHidden = !wasVisible;
            skillIndex = i;
            break;
        }
    }

    if (skillIndex == -1) {
        qCWarning(logAIGUI) << "Skill not found in kCustomFunctionList:" << name;
        return;
    }

    item->setSkillVisible(!WordWizard::kCustomFunctionList[skillIndex].isHidden);

    if (item->isCustomSkill()) {
        WordWizard::saveCustomFunctions();
        emit skillListChanged();
        return;
    }

    if (WordWizard::kCustomFunctionList[skillIndex].isHidden && currentIndex >= 0) {
        qCDebug(logAIGUI) << "Hiding skill, moving to end";
        if (m_isHideAnimating) {
            m_pendingHideOperations.enqueue(qMakePair(name, currentIndex));
            return;
        }

        m_isHideAnimating = true;
        animateSkillToEnd(item, currentIndex);
    } else if (!WordWizard::kCustomFunctionList[skillIndex].isHidden) {
        if (m_isHideAnimating) {
            m_pendingHideOperations.enqueue(qMakePair(name, currentIndex));
            return;
        }
        m_isHideAnimating = true;
        animateSkillToEnabledEnd(item);
    } else {
        WordWizard::saveCustomFunctions();
        emit skillListChanged();
    }
}

void SkillListWidget::animateSkillToEnd(SkillListItem *item, int fromIndex)
{

    QString skillName = item->getName();
    int targetIndex = m_skillItems.size() - 1;

    if (fromIndex == targetIndex) {
        m_isHideAnimating = false;
        processNextHideOperation();
        return;
    }

    CustomFunction skillInfo = WordWizard::kCustomFunctionList.takeAt(fromIndex);
    WordWizard::kCustomFunctionList.append(skillInfo);

    SkillListItem *movedItem = m_skillItems.takeAt(fromIndex);
    m_skillItems.append(movedItem);

    if (m_animationGroup) {
        m_animationGroup->stop();
        m_animationGroup->deleteLater();
        m_animationGroup = nullptr;
    }

    m_animationGroup = new QParallelAnimationGroup(this);
    connect(m_animationGroup, &QParallelAnimationGroup::finished, [this]() {
        onHideAnimationFinished();
    });

    // 为被移动的item创建动画到目标位置
    QRect targetRect = getItemRect(targetIndex);
    QPropertyAnimation *mainAnimation = new QPropertyAnimation(item, "geometry");
    mainAnimation->setDuration(ANIMATION_DURATION);
    mainAnimation->setEasingCurve(QEasingCurve::OutCubic);
    mainAnimation->setStartValue(item->geometry());
    mainAnimation->setEndValue(targetRect);
    m_animationGroup->addAnimation(mainAnimation);

    // 为其他需要移动的item创建动画到新位置
    for (int i = 0; i < m_skillItems.size(); ++i) {
        SkillListItem *otherItem = m_skillItems[i];
        if (otherItem && otherItem != item) {
            QRect newRect = getItemRect(i);

            QPropertyAnimation *animation = new QPropertyAnimation(otherItem, "geometry");
            animation->setDuration(ANIMATION_DURATION);
            animation->setEasingCurve(QEasingCurve::OutCubic);
            animation->setStartValue(otherItem->geometry());
            animation->setEndValue(newRect);
            m_animationGroup->addAnimation(animation);
        }
    }

    // 启动动画
    m_animationGroup->start();

    // 保存到配置文件
    WordWizard::saveCustomFunctions();
    emit skillListChanged();
}

void SkillListWidget::onHideAnimationFinished()
{
    // 清理动画组
    if (m_animationGroup) {
        m_animationGroup->deleteLater();
        m_animationGroup = nullptr;
    }

    // 处理队列中的下一个操作
    processNextHideOperation();
}

void SkillListWidget::processNextHideOperation()
{
    if (!m_pendingHideOperations.isEmpty()) {
        auto operation = m_pendingHideOperations.dequeue();
        QString skillName = operation.first;

        // 重新查找当前的索引位置，因为数据结构已经发生变化
        SkillListItem *item = nullptr;
        int currentIndex = -1;
        for (int i = 0; i < m_skillItems.size(); ++i) {
            if (m_skillItems[i]->getName() == skillName) {
                item = m_skillItems[i];
                currentIndex = i;
                break;
            }
        }

        if (item && !item->isCustomSkill() && currentIndex >= 0) {
            if (!item->isSkillVisible()) {
                animateSkillToEnd(item, currentIndex);
            } else {
                animateSkillToEnabledEnd(item);
            }
        } else {
            // 如果条件不满足，继续处理下一个
            processNextHideOperation();
        }
    } else {
        m_isHideAnimating = false;
    }
}

void SkillListWidget::refreshSkillList()
{
    for (auto item : m_skillItems) {
        item->deleteLater();
    }
    m_skillItems.clear();

    if (!WordWizard::kCustomFunctionList.isEmpty()) {
        sortSkillsByEnabledState();
        
        for (int i = 0; i < WordWizard::kCustomFunctionList.size(); ++i) {
            const CustomFunction &skillInfo = WordWizard::kCustomFunctionList[i];
            SkillListItem *item = createSkillItem(skillInfo);

            if (item) {
                m_skillItems.append(item);
                item->show();
            }
        }

        updateItemPositions(false);

        int totalHeight = calculateTotalHeight();
        m_pHasSkillWidget->setFixedHeight(totalHeight);
    }
}

void SkillListWidget::updateItemPositions(bool animated)
{
    if (m_skillItems.isEmpty()) return;

    // 停止之前的动画
    if (m_animationGroup) {
        m_animationGroup->stop();
        m_animationGroup->deleteLater();
        m_animationGroup = nullptr;
    }

    if (animated) {
        m_animationGroup = new QParallelAnimationGroup(this);
        connect(m_animationGroup, &QParallelAnimationGroup::finished,
                this, &SkillListWidget::onAnimationFinished);
    }

    // 为每个item设置位置
    for (int i = 0; i < m_skillItems.size(); ++i) {
        SkillListItem *item = m_skillItems[i];
        if (!item) continue;

        // 在拖拽过程中跳过被拖拽的item，但拖拽完成后需要更新所有item
        if (item == m_draggedItem && m_isDragging) continue;

        QRect targetRect = getItemRect(i);

        if (animated && item->geometry() != targetRect) {
            QPropertyAnimation *animation = new QPropertyAnimation(item, "geometry");
            animation->setDuration(ANIMATION_DURATION);
            animation->setEasingCurve(QEasingCurve::OutCubic);
            animation->setStartValue(item->geometry());
            animation->setEndValue(targetRect);
            m_animationGroup->addAnimation(animation);
        } else {
            item->setGeometry(targetRect);
        }
    }

    if (animated && m_animationGroup && m_animationGroup->animationCount() > 0) {
        m_animationGroup->start();
    }
}

void SkillListWidget::animateItemToPosition(SkillListItem *item, int targetIndex)
{
    if (!item || targetIndex < 0 || targetIndex >= m_skillItems.size()) return;

    QRect targetRect = getItemRect(targetIndex);

    QPropertyAnimation *animation = new QPropertyAnimation(item, "geometry");
    animation->setDuration(ANIMATION_DURATION);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->setStartValue(item->geometry());
    animation->setEndValue(targetRect);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QRect SkillListWidget::getItemRect(int index) const
{
    if (index < 0) return QRect();

    int y = 8 + index * (ITEM_HEIGHT + ITEM_SPACING);  // 增加顶部边距
    int x = (m_pHasSkillWidget->width() - ITEM_WIDTH) / 2;  // 居中显示

    return QRect(x, y, ITEM_WIDTH, ITEM_HEIGHT);
}

int SkillListWidget::calculateDropIndex(const QPoint &pos)
{
    if (!m_pHasSkillWidget) return -1;

    QPoint localPos = m_pHasSkillWidget->mapFromGlobal(pos);
    int y = localPos.y() - 8;  // 考虑顶部边距

    // 计算应该插入的位置
    int index = (y + ITEM_SPACING / 2) / (ITEM_HEIGHT + ITEM_SPACING);

    // 边界检查
    if (index < 0) index = 0;
    if (index > WordWizard::kCustomFunctionList.size()) index = WordWizard::kCustomFunctionList.size();

    int enabledCount = getEnabledSkillCount();

    if (m_draggedIndex >= 0 && m_draggedIndex < WordWizard::kCustomFunctionList.size()) {
        const CustomFunction &draggedSkill = WordWizard::kCustomFunctionList[m_draggedIndex];
        
        if (!draggedSkill.isHidden) {
            if (index > enabledCount) {
                index = enabledCount;
            }
        } else {
            if (index < enabledCount) {
                index = enabledCount;
            }
        }
    }

    return index;
}

void SkillListWidget::onDragStarted(SkillListItem *item, const QPoint &startPos)
{
    if (m_isDragging || !item) {
        qCWarning(logAIGUI) << "Invalid drag start: already dragging or null item";
        return;
    }

    if (m_isHideAnimating) {
        if (m_animationGroup) {
            m_animationGroup->stop();
            m_animationGroup->deleteLater();
            m_animationGroup = nullptr;
        }
        m_isHideAnimating = false;
        m_pendingHideOperations.clear();
        updateItemPositions(false);
    }

    m_isDragging = true;
    m_draggedItem = item;
    m_draggedIndex = m_skillItems.indexOf(item);
    m_dropIndex = m_draggedIndex;
    m_dragStartPos = startPos;

    QPoint itemPos = item->mapToGlobal(QPoint(0, 0));
    m_dragOffset = startPos - itemPos;

    item->raise();
}

void SkillListWidget::onDragMoved(const QPoint &currentPos)
{
    if (!m_isDragging || !m_draggedItem) {
        qCWarning(logAIGUI) << "Invalid drag move: not dragging or null dragged item";
        return;
    }

    // 更新被拖拽item的位置
    QPoint newPos = currentPos - m_dragOffset;
    QPoint localPos = m_pHasSkillWidget->mapFromGlobal(newPos);

    // 限制在容器范围内，考虑边距和居中
    int maxY = m_pHasSkillWidget->height() - ITEM_HEIGHT - 8;
    localPos.setY(qBound(8, localPos.y(), maxY));

    // 水平居中
    int centerX = (m_pHasSkillWidget->width() - ITEM_WIDTH) / 2;
    localPos.setX(centerX);

    m_draggedItem->move(localPos);

    // 计算新的drop位置
    int newDropIndex = calculateDropIndex(currentPos);

    if (newDropIndex != m_dropIndex) {
        m_dropIndex = newDropIndex;

        // 创建临时的技能列表来预览排序效果
        QList<CustomFunction> tempList = WordWizard::kCustomFunctionList;
        if (m_draggedIndex >= 0 && m_draggedIndex < tempList.size()) {
            CustomFunction draggedSkill = tempList.takeAt(m_draggedIndex);
            int insertIndex = m_dropIndex;
            if (insertIndex > m_draggedIndex) {
                insertIndex--;  // 因为移除了一个元素，插入位置需要调整
            }
            insertIndex = qBound(0, insertIndex, tempList.size());
            tempList.insert(insertIndex, draggedSkill);
        }

        // 更新其他item的位置（动画）
        for (int i = 0; i < m_skillItems.size(); ++i) {
            SkillListItem *item = m_skillItems[i];
            if (item == m_draggedItem) continue;

            QString itemName = item->getName();
            int newIndex = -1;
            for (int j = 0; j < tempList.size(); ++j) {
                const CustomFunction &skill = tempList[j];
                QString skillDisplayName = skill.isCustom ? skill.name : WordWizard::getDefaultSkillName(skill.defaultFunctionType);
                if (skillDisplayName == itemName) {
                    newIndex = j;
                    break;
                }
            }
            if (newIndex >= 0) {
                animateItemToPosition(item, newIndex);
            }
        }
    }
}

void SkillListWidget::onDragFinished()
{
    if (!m_isDragging) {
        qCWarning(logAIGUI) << "Drag finished called but not currently dragging";
        return;
    }

    if (!DConfigManager::instance()->contains(WORDWIZARD_GROUP, WORDWIZARD_CUSTOM_FUNCTIONS)) {
        // DConfig未实时刷新时，直接恢复位置
        m_isDragging = false;
        m_draggedItem = nullptr;
        m_draggedIndex = -1;
        m_dropIndex = -1;
        updateItemPositions(true);
        // 触发弹窗
        DConfigManager::checkConfigAvailable(WORDWIZARD_GROUP, WORDWIZARD_CUSTOM_FUNCTIONS);
        return;
    }

    // 如果位置发生了变化，更新技能列表
    if (m_draggedIndex >= 0 && m_dropIndex >= 0 && m_draggedIndex != m_dropIndex) {
        if (m_draggedIndex >= WordWizard::kCustomFunctionList.size()) {
            qCWarning(logAIGUI) << "Invalid drag index:" << m_draggedIndex;
            m_isDragging = false;
            m_draggedItem = nullptr;
            m_draggedIndex = -1;
            m_dropIndex = -1;
            return;
        }

        // 更新CustomFunction列表顺序
        CustomFunction draggedSkillInfo = WordWizard::kCustomFunctionList.takeAt(m_draggedIndex);
        int insertIndex = m_dropIndex;
        if (insertIndex > m_draggedIndex) {
            insertIndex--;  // 因为移除了一个元素，插入位置需要调整
        }
        insertIndex = qBound(0, insertIndex, WordWizard::kCustomFunctionList.size());
        WordWizard::kCustomFunctionList.insert(insertIndex, draggedSkillInfo);

        // 同步更新m_skillItems列表的顺序
        SkillListItem *draggedItem = m_skillItems.takeAt(m_draggedIndex);
        m_skillItems.insert(insertIndex, draggedItem);
        for (int i = 0; i < m_skillItems.size(); ++i) {
            qDebug() << "  " << i << ":" << m_skillItems[i]->getName();
        }

        // 重置拖拽状态，然后立即更新所有item的位置
        m_isDragging = false;
        m_draggedItem = nullptr;
        m_draggedIndex = -1;
        m_dropIndex = -1;

        // 立即更新所有item的位置到正确的最终位置
        updateItemPositions(true);

        // 保存到配置文件
        WordWizard::saveCustomFunctions();
        emit skillListChanged();

    } else {
        // 如果没有移动，直接恢复位置
        m_isDragging = false;
        m_draggedItem = nullptr;
        m_draggedIndex = -1;
        m_dropIndex = -1;
        updateItemPositions(true);
    }
}

void SkillListWidget::onAnimationFinished()
{
    if (m_animationGroup) {
        m_animationGroup->deleteLater();
        m_animationGroup = nullptr;
    }
}

int SkillListWidget::getEnabledSkillCount() const
{
    int count = 0;
    for (const CustomFunction &skill : WordWizard::kCustomFunctionList) {
        if (!skill.isHidden) {
            count++;
        }
    }
    return count;
}

void SkillListWidget::animateSkillToEnabledEnd(SkillListItem *item)
{
    QString skillName = item->getName();

    int currentIndex = -1;
    for (int i = 0; i < m_skillItems.size(); ++i) {
        if (m_skillItems[i] == item) {
            currentIndex = i;
            break;
        }
    }
    
    if (currentIndex == -1) {
        m_isHideAnimating = false;
        processNextHideOperation();
        return;
    }

    int enabledCount = getEnabledSkillCount();

    int targetIndex = enabledCount - 1;

    if (currentIndex == targetIndex) {
        m_isHideAnimating = false;
        processNextHideOperation();
        return;
    }

    CustomFunction skillInfo = WordWizard::kCustomFunctionList.takeAt(currentIndex);
    WordWizard::kCustomFunctionList.insert(targetIndex, skillInfo);

    SkillListItem *movedItem = m_skillItems.takeAt(currentIndex);
    m_skillItems.insert(targetIndex, movedItem);

    if (m_animationGroup) {
        m_animationGroup->stop();
        m_animationGroup->deleteLater();
        m_animationGroup = nullptr;
    }

    m_animationGroup = new QParallelAnimationGroup(this);
    connect(m_animationGroup, &QParallelAnimationGroup::finished, [this]() {
        onHideAnimationFinished();
    });

    for (int i = 0; i < m_skillItems.size(); ++i) {
        SkillListItem *currentItem = m_skillItems[i];
        if (!currentItem) continue;

        QRect targetRect = getItemRect(i);
        QPropertyAnimation *animation = new QPropertyAnimation(currentItem, "geometry");
        animation->setDuration(ANIMATION_DURATION);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        animation->setStartValue(currentItem->geometry());
        animation->setEndValue(targetRect);
        m_animationGroup->addAnimation(animation);
    }

    m_animationGroup->start();

    WordWizard::saveCustomFunctions();
    emit skillListChanged();
}

void SkillListWidget::sortSkillsByEnabledState()
{
    if (WordWizard::kCustomFunctionList.isEmpty()) {
        return;
    }

    // 排序，启用技能在前，隐藏技能在后
    std::stable_sort(WordWizard::kCustomFunctionList.begin(), WordWizard::kCustomFunctionList.end(),
                     [](const CustomFunction &a, const CustomFunction &b) {
                         if (a.isHidden != b.isHidden) {
                             return !a.isHidden;
                         }
                         return false;
                     });
}

int SkillListWidget::calculateTotalHeight() const
{
    if (WordWizard::kCustomFunctionList.isEmpty()) {
        return 100; // 最小高度
    }

    int totalSkills = WordWizard::kCustomFunctionList.size();
    int height = totalSkills * ITEM_HEIGHT + (totalSkills - 1) * ITEM_SPACING + 16; // 增加上下边距
    
    return height;
}

void SkillListWidget::resizeEvent(QResizeEvent *event)
{
    DWidget::resizeEvent(event);

    if (!m_isDragging) {
        updateItemPositions(false);
    }
}
