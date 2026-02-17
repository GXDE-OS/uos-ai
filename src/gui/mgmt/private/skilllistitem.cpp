#include "skilllistitem.h"
#include <dconfigmanager.h>
#include <QPainter>
#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DStyle>
#include <QMetaObject>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
using namespace uos_ai;

SkillListItem::SkillListItem(const QString &name, const QString &iconName, DWidget *parent)
    : DWidget(parent), m_name(name), m_iconName(iconName), m_isDragging(false), m_isCustomSkill(false)
{
    setObjectName("SkillListItem_" + name);
    initUI();
}

void SkillListItem::initUI()
{
    setFixedSize(560, 40);
    setMouseTracking(true);
}

void SkillListItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();

    QRect bgRect = rect().adjusted(1, 1, -1, -1);
    
    QColor bgColor;
    QColor borderColor;
    
    if (m_isDragging) {
        bgColor = palette.color(DPalette::ItemBackground);
        bgColor.setAlpha(12); // 0.05
        borderColor = palette.color(DPalette::FrameBorder);
        painter.setPen(QPen(borderColor, 1));
    } else if (m_isHovered) {
        bgColor = palette.color(DPalette::ItemBackground).lighter(105);
        bgColor.setAlpha(12); // 0.05
        borderColor = palette.color(DPalette::FrameBorder);
        painter.setPen(QPen(borderColor, 1));
    } else {
        bgColor = palette.color(DPalette::ItemBackground);
        bgColor.setAlpha(7); // 0.03
        borderColor = palette.color(DPalette::FrameBorder);
        painter.setPen(QPen(borderColor, 1));
    }

    painter.setBrush(bgColor);
    painter.drawRoundedRect(bgRect, 6, 6);

    drawDragHandle(painter, getDragHandleRect());
    drawIcon(painter, getIconRect());
    drawName(painter, getNameRect());

    if (m_isHovered) {
        if (m_isCustomSkill) {
            drawEditButton(painter, getEditButtonRect());
            drawDeleteButton(painter, getDeleteButtonRect());
        } else {
            drawHideButton(painter, getHideButtonRect());
        }
    }

    if (shouldShowGrayed()) {
        painter.setPen(Qt::NoPen);
        QColor grayMask = palette.color(DPalette::Window);
        if (m_isHovered)
            grayMask.setAlpha(128);  // 50% 透明度
        else
            grayMask.setAlpha(153);  // 40% 透明度
        painter.setBrush(grayMask);
        painter.drawRoundedRect(bgRect, 6, 6);

        if (m_isHovered && !m_isCustomSkill && !m_skillVisible) {
            drawHideButton(painter, getHideButtonRect());
        }
    }
}

QRect SkillListItem::getDragHandleRect() const
{
    return QRect(MARGIN, (height() - DRAG_HANDLE_WIDTH) / 2, DRAG_HANDLE_WIDTH, DRAG_HANDLE_WIDTH);
}

QRect SkillListItem::getIconRect() const
{
    int x = MARGIN + DRAG_HANDLE_WIDTH + SPACING;
    return QRect(x, (height() - ICON_SIZE) / 2 + 1, ICON_SIZE, ICON_SIZE);
}

QRect SkillListItem::getNameRect() const
{
    int x = MARGIN + DRAG_HANDLE_WIDTH + SPACING + ICON_SIZE + SPACING - 2;
    int width = this->width() - x - MARGIN;

    if (m_isHovered) {
        if (m_isCustomSkill) {
            width -= (EDIT_BUTTON_SIZE + SPACING + DELETE_BUTTON_SIZE + SPACING);
        } else {
            QString buttonText = m_skillVisible ? tr("Disabled") : tr("Enable");
            QFontMetrics fm(font());
            int textWidth = fm.horizontalAdvance(buttonText);
            int buttonWidth = textWidth + 8;
            width -= (buttonWidth + SPACING);
        }
    }
    return QRect(x, 0, width, height());
}

QRect SkillListItem::getHideButtonRect() const
{
    QString buttonText = m_skillVisible ? tr("Disabled") : tr("Enable");

    QFontMetrics fm(font());
    int textWidth = fm.horizontalAdvance(buttonText);
    int buttonWidth = textWidth + 8;
    int buttonHeight = 20;
    
    int x = width() - MARGIN - buttonWidth;
    return QRect(x, (height() - buttonHeight) / 2, buttonWidth, buttonHeight);
}

QRect SkillListItem::getEditButtonRect() const
{
    int x = width() - MARGIN - EDIT_BUTTON_SIZE + 2;
    return QRect(x, (height() - EDIT_BUTTON_SIZE) / 2, EDIT_BUTTON_SIZE, EDIT_BUTTON_SIZE);
}

QRect SkillListItem::getDeleteButtonRect() const
{
    int x = width() - MARGIN - EDIT_BUTTON_SIZE - SPACING * 2 - DELETE_BUTTON_SIZE;
    return QRect(x, (height() - DELETE_BUTTON_SIZE) / 2, DELETE_BUTTON_SIZE, DELETE_BUTTON_SIZE);
}

void SkillListItem::drawDragHandle(QPainter &painter, const QRect &rect)
{
    painter.setPen(Qt::NoPen);

    QColor dotColor;
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();

    if (themeType == DGuiApplicationHelper::DarkType) {
        dotColor = QColor(255, 255, 255, 77);  // rgba(255, 255, 255, 0.3)
    } else {
        dotColor = QColor(0, 0, 0, 77);  // rgba(0, 0, 0, 0.3)
    }
    painter.setBrush(dotColor);
    
    int dotSize = 2;
    int spacing = 3;

    int totalWidth = 2 * dotSize + spacing;
    int totalHeight = 3 * dotSize + 2 * spacing;
    int startX = rect.x() + (rect.width() - totalWidth) / 2;
    int startY = rect.y() + (rect.height() - totalHeight) / 2;
    
    // 绘制2x3的点阵
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 2; ++col) {
            int x = startX + col * (dotSize + spacing);
            int y = startY + row * (dotSize + spacing);
            painter.drawEllipse(x, y, dotSize, dotSize);
        }
    }
}

void SkillListItem::drawIcon(QPainter &painter, const QRect &rect)
{
    QIcon icon;

    if (!m_iconName.isEmpty()) {
        icon = QIcon::fromTheme(m_iconName);
    }

    QIcon::Mode iconMode = QIcon::Normal;
    QIcon::State iconState = QIcon::Off;

    if (shouldShowGrayed() || (!m_skillVisible && !m_isCustomSkill)) {
        iconMode = QIcon::Disabled;
    }

    int margin = 2;
    QRect iconRect = rect.adjusted(margin, margin, -margin, -margin);
    icon.paint(&painter, iconRect, Qt::AlignCenter, iconMode, iconState);
}

void SkillListItem::drawName(QPainter &painter, const QRect &rect)
{
    DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
    QColor textColor = palette.color(DPalette::Text);
    painter.setPen(textColor);
    
    QFont font = painter.font();
    painter.setFont(font);
    
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, m_name);
}

void SkillListItem::drawHideButton(QPainter &painter, const QRect &rect)
{
    QString buttonText = m_skillVisible ? tr("Disabled") : tr("Enable");

    DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
    QColor textColor = palette.color(DPalette::Highlight);
    
    if (m_hideButtonHovered) {
        textColor.setAlpha(180);  // 约70%透明度
    } else {
        textColor.setAlpha(255);
    }
    
    painter.setPen(textColor);

    QFont buttonFont = font();
    painter.setFont(buttonFont);

    painter.drawText(rect, Qt::AlignCenter, buttonText);
}

void SkillListItem::drawEditButton(QPainter &painter, const QRect &rect)
{
    QIcon editIcon = QIcon::fromTheme("uos-ai-assistant_edit");
    
    if (!editIcon.isNull()) {
        if (m_editButtonHovered) {
            DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
            QColor highlightColor = palette.color(DPalette::Highlight);
            
            QPixmap pixmap = editIcon.pixmap(rect.size());
            QPainter iconPainter(&pixmap);
            iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            iconPainter.fillRect(pixmap.rect(), highlightColor);
            iconPainter.end();
            
            QIcon coloredIcon(pixmap);
            coloredIcon.paint(&painter, rect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        } else {
            editIcon.paint(&painter, rect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }
    }
}

void SkillListItem::drawDeleteButton(QPainter &painter, const QRect &rect)
{
    QIcon deleteIcon = QIcon::fromTheme("edit-delete");

    if (deleteIcon.isNull()) {
        deleteIcon = DStyle::standardIcon(style(), DStyle::SP_DeleteButton);
    }
    
    if (!deleteIcon.isNull()) {
        if (m_deleteButtonHovered) {
            DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
            QColor highlightColor = palette.color(DPalette::Highlight);
            
            QPixmap pixmap = deleteIcon.pixmap(rect.size());
            QPainter iconPainter(&pixmap);
            iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            iconPainter.fillRect(pixmap.rect(), highlightColor);
            iconPainter.end();
            
            QIcon coloredIcon(pixmap);
            coloredIcon.paint(&painter, rect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        } else {
            deleteIcon.paint(&painter, rect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }
    }
}

void SkillListItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        bool clickedOnButton = false;
        QString targetName = m_name;
        
        if (m_isHovered) {
            if (m_isCustomSkill) {
                QRect editRect = getEditButtonRect();
                QRect deleteRect = getDeleteButtonRect();
                
                if (editRect.contains(event->pos())) {
                    clickedOnButton = true;
                    QMetaObject::invokeMethod(this, "signalEditItem", Qt::QueuedConnection, Q_ARG(QString, targetName));
                    
                } else if (deleteRect.contains(event->pos())) {
                    clickedOnButton = true;
                    QMetaObject::invokeMethod(this, "signalDeleteItem", Qt::QueuedConnection, Q_ARG(QString, targetName));
                }
            } else {
                QRect hideRect = getHideButtonRect();
                if (hideRect.contains(event->pos())) {
                    if (DConfigManager::checkConfigAvailable(WORDWIZARD_GROUP, WORDWIZARD_CUSTOM_FUNCTIONS)) {
                        m_skillVisible = !m_skillVisible;
                        update();
                        clickedOnButton = true;
                        QMetaObject::invokeMethod(this, "signalHideItem", Qt::QueuedConnection, Q_ARG(QString, targetName));
                    } else {
                        clickedOnButton = true;
                    }
                }
            }
        }

        if (!clickedOnButton && !shouldShowGrayed()) {
            m_dragStartPosition = event->pos();
            m_isDragging = false;
        } else {
            m_dragStartPosition = QPoint();
        }
    }
    DWidget::mousePressEvent(event);
}

void SkillListItem::mouseMoveEvent(QMouseEvent *event)
{
    bool inButtonArea = false;
    
    if (m_isHovered) {
        if (m_isCustomSkill) {
            QRect editRect = getEditButtonRect();
            QRect deleteRect = getDeleteButtonRect();
            
            bool wasEditHovered = m_editButtonHovered;
            bool wasDeleteHovered = m_deleteButtonHovered;
            
            m_editButtonHovered = editRect.contains(event->pos());
            m_deleteButtonHovered = deleteRect.contains(event->pos());
            
            if (wasEditHovered != m_editButtonHovered) {
                update(editRect);
            }
            if (wasDeleteHovered != m_deleteButtonHovered) {
                update(deleteRect);
            }
            
            inButtonArea = m_editButtonHovered || m_deleteButtonHovered;
        } else {
            QRect hideRect = getHideButtonRect();
            bool wasHovered = m_hideButtonHovered;
            m_hideButtonHovered = hideRect.contains(event->pos());
            if (wasHovered != m_hideButtonHovered) {
                update(hideRect);
            }
            
            inButtonArea = m_hideButtonHovered;
        }
    }

    if (!inButtonArea && !shouldShowGrayed() && !m_isDragging) {
        setCursor(Qt::OpenHandCursor);
    } else if (!m_isDragging) {
        setCursor(Qt::ArrowCursor);
    }

    if (!(event->buttons() & Qt::LeftButton))
        return;

    if (m_dragStartPosition.isNull())
        return;
    
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
        return;

    if (!shouldShowGrayed()) {
        bool dragStartInButtonArea = false;
        
        if (m_isHovered) {
            if (m_isCustomSkill) {
                QRect editRect = getEditButtonRect();
                QRect deleteRect = getDeleteButtonRect();
                dragStartInButtonArea = editRect.contains(m_dragStartPosition) || deleteRect.contains(m_dragStartPosition);
            } else {
                QRect hideRect = getHideButtonRect();
                dragStartInButtonArea = hideRect.contains(m_dragStartPosition);
            }
        }
        
        if (!dragStartInButtonArea) {
            if (!m_isDragging) {
                m_isDragging = true;
                setCursor(Qt::ClosedHandCursor);
                raise();
                setAutoFillBackground(true);
                emit signalDragStarted(this, mapToGlobal(event->pos()));
                update();
            } else {
                emit signalDragMoved(mapToGlobal(event->pos()));
            }
        }
    }
}

void SkillListItem::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        m_isDragging = false;
        m_dragStartPosition = QPoint();

        bool inButtonArea = false;
        
        if (m_isHovered) {
            if (m_isCustomSkill) {
                QRect editRect = getEditButtonRect();
                QRect deleteRect = getDeleteButtonRect();
                inButtonArea = editRect.contains(event->pos()) || deleteRect.contains(event->pos());
            } else {
                QRect hideRect = getHideButtonRect();
                inButtonArea = hideRect.contains(event->pos());
            }
        }

        if (!inButtonArea && !shouldShowGrayed()) {
            setCursor(Qt::OpenHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }

        setAutoFillBackground(false);
        emit signalDragFinished();
        update();
    }
    DWidget::mouseReleaseEvent(event);
}

#ifdef COMPILE_ON_QT6
void SkillListItem::enterEvent(QEnterEvent *event)
#else
void SkillListItem::enterEvent(QEvent *event)
#endif
{
    m_isHovered = true;

    QPoint pos = mapFromGlobal(QCursor::pos());

    bool inButtonArea = false;
    
    if (m_isCustomSkill) {
        QRect editRect = getEditButtonRect();
        QRect deleteRect = getDeleteButtonRect();
        inButtonArea = editRect.contains(pos) || deleteRect.contains(pos);
    } else {
        QRect hideRect = getHideButtonRect();
        inButtonArea = hideRect.contains(pos);
    }

    if (!inButtonArea && !shouldShowGrayed()) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    
    update();
    DWidget::enterEvent(event);
}

void SkillListItem::leaveEvent(QEvent *event)
{
    m_isHovered = false;
    m_hideButtonHovered = false;
    m_editButtonHovered = false;
    m_deleteButtonHovered = false;

    if (!m_isDragging) {
        m_dragStartPosition = QPoint();
    }
    
    setCursor(Qt::ArrowCursor);
    update();
    DWidget::leaveEvent(event);
}
