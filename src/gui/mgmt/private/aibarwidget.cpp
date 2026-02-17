#include "aibarwidget.h"
#include "themedlable.h"
#include "aibaritem.h"

#include <DLabel>
#include <DFontSizeManager>
#include <DBackgroundGroup>
#include <DGuiApplicationHelper>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLoggingCategory>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

AiBarWidget::AiBarWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    onThemeTypeChanged();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &AiBarWidget::onThemeTypeChanged);
}

void AiBarWidget::initUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_pWidgetLabel = new ThemedLable(tr("DDE Shell AI Bar"));
    m_pWidgetLabel->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_pWidgetLabel, DFontSizeManager::T5, QFont::Bold);

    layout->addWidget(m_pWidgetLabel, 0, Qt::AlignLeft);
    layout->addWidget(aiBarWidget());
    layout->addStretch();
}

void AiBarWidget::onThemeTypeChanged()
{
    qCDebug(logAIGUI) << "Theme type changed, updating palette.";
    DPalette pl = m_pAiBarWidget->palette();
    pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pAiBarWidget->setPalette(pl);
}

DBackgroundGroup *AiBarWidget::aiBarWidget()
{
    m_AiBarItem = new AiBarItem(this);
    m_AiBarItem->setText(tr("Drag files quickly"), tr("Drag files to the AI ribbon for intelligent processing"));
    m_AiBarItem->setMinimumHeight(60);
    m_AiBarItem->setMaximumHeight(75);
    m_AiBarItem->setSwitchChecked(true);

    connect(m_AiBarItem, &AiBarItem::signalSwitchChanged, this, &AiBarWidget::signalChangeDragStatus);

    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(m_AiBarItem);

    m_pAiBarWidget = new DBackgroundGroup(bgLayout, this);
    m_pAiBarWidget->setContentsMargins(0, 0, 0, 0);
    return m_pAiBarWidget;
}

QString AiBarWidget::getTitleName()
{
    return m_pWidgetLabel->text();
}

void AiBarWidget::updateDragStatus(bool enable)
{
    qCDebug(logAIGUI) << "Updating drag status to" << enable;
    m_AiBarItem->setSwitchChecked(enable);
}
