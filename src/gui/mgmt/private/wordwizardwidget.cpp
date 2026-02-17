#include "wordwizardwidget.h"
#include "themedlable.h"
#include "wordwizarditem.h"
#include "disableappwidget.h"
#include "skilllistwidget.h"
#include "util.h"
#include "wizardwrapperlabel.h"

#include <DLabel>
#include <DFontSizeManager>
#include <DBackgroundGroup>
#include <DGuiApplicationHelper>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLoggingCategory>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

WordWizardWidget::WordWizardWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    initConnect();
    onThemeTypeChanged();
}

void WordWizardWidget::initUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 0, 0, 0);
    layout->setSpacing(10);

    m_pWidgetLabel = new ThemedLable(tr("UOS AI FollowAlong"));
    m_pWidgetLabel->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_pWidgetLabel, DFontSizeManager::T5, QFont::Bold);

    m_pSkillListWidget = new SkillListWidget(this);
    m_pSkillListWidget->setFixedWidth(560);

    layout->addWidget(m_pWidgetLabel, 0, Qt::AlignLeft);
    layout->addWidget(wordWizardWidget(), 0, Qt::AlignLeft);
    layout->addWidget(m_pSkillListWidget);
    layout->addWidget(disableAppWidget());
    layout->addStretch();

    if (m_pDisableWidget && m_pDisableWidget->isEmpty()) {
        m_pDisableAppWidget->setVisible(false);
    }

    setLayout(layout);
}

void WordWizardWidget::initConnect()
{
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &WordWizardWidget::onThemeTypeChanged);

    connect(m_pSkillListWidget, &SkillListWidget::skillListChanged, m_imageLabel, &WizardWrapperLabel::onSkillListChanged);

    connect(m_pSkillListWidget, &SkillListWidget::skillAddedSuccessfully, this, &WordWizardWidget::skillAddedSuccessfully);

    connect(m_WordWizardItem, &WordWizardItem::signalSwitchChanged, this, &WordWizardWidget::signalChangeHiddenStatus);

    connect(m_pDisableWidget, &DisableAppWidget::becameEmpty, this, &WordWizardWidget::onDisableWidgetEmpty);
    connect(m_pDisableWidget, &DisableAppWidget::appAdded, this, &WordWizardWidget::onDisableAppAdded);
    connect(m_pDisableWidget, &DisableAppWidget::requestDisabledAppsUpdate,
            this, &WordWizardWidget::disabledAppsUpdateRequested);
}

void WordWizardWidget::onThemeTypeChanged()
{
    qCDebug(logAIGUI) << "Theme type changed, updating UI";
    DPalette pl = m_pWordWizardWidget->palette();
    pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pWordWizardWidget->setPalette(pl);
}

DBackgroundGroup *WordWizardWidget::wordWizardWidget()
{
    m_imageLabel = new WizardWrapperLabel(this);
    m_imageLabel->setContentsMargins(0, 0, 0, 0);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setRadius(8, 8);

    m_WordWizardItem = new WordWizardItem(this);
    m_WordWizardItem->setText(tr("Automatically display the UOS AI FollowAlong toolbar when text is selected."), tr("After closing, the selected text can still be woken up by the shortcut Super+R bar."));
    m_WordWizardItem->setMinimumHeight(60);
    m_WordWizardItem->setMaximumHeight(75);
    m_WordWizardItem->setSwitchChecked(true);

    DWidget *widget = new DWidget(this);
    auto *bgLayout = new QVBoxLayout(widget);
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->setSpacing(10);

    QWidget *imageWrapper = m_imageLabel->createWrapper(this);

    bgLayout->addWidget(imageWrapper, 0, Qt::AlignCenter);
    bgLayout->addWidget(m_WordWizardItem);

    m_pWordWizardWidget = new DBackgroundGroup(bgLayout, this);
    m_pWordWizardWidget->setItemSpacing(1);
    m_pWordWizardWidget->setContentsMargins(0, 0, 0, 0);
    m_pWordWizardWidget->setFixedWidth(560);

    return m_pWordWizardWidget;
}

DBackgroundGroup *WordWizardWidget::disableAppWidget()
{
    DWidget *widget = new DWidget(this);
    auto *bgLayout = new QVBoxLayout(widget);
    bgLayout->setContentsMargins(0, 0, 0, 0);

    m_pDisableWidget = new DisableAppWidget(widget);
    m_pDisableWidget->setFixedWidth(568);
    bgLayout->addWidget(m_pDisableWidget);

    m_pDisableAppWidget = new DBackgroundGroup(bgLayout, this);
    m_pDisableAppWidget->setContentsMargins(0, 0, 0, 0);
    m_pDisableAppWidget->setFixedWidth(576);

    return m_pDisableAppWidget;
}

QString WordWizardWidget::getTitleName()
{
    return m_pWidgetLabel->text();
}

void WordWizardWidget::updateHiddenStatus(bool isHidden)
{
    qCDebug(logAIGUI) << "Updating hidden status:" << isHidden;
    m_WordWizardItem->setSwitchChecked(isHidden);
}

void WordWizardWidget::onDisableWidgetEmpty()
{
    qCDebug(logAIGUI) << "Disable widget became empty";
    if (m_pDisableAppWidget) {
        if (m_WordWizardItem) {
            m_WordWizardItem->setFocus();
        } else {
            this->setFocus();
        }

        m_pDisableAppWidget->setVisible(false);
    }
}

void WordWizardWidget::onDisableAppAdded(const QString &appName)
{
    if (m_pDisableAppWidget && !m_pDisableAppWidget->isVisible()) {
        m_pDisableAppWidget->setVisible(true);
    }
    emit disabledAppAdded(appName);
}
