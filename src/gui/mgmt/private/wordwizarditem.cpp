#include "wordwizarditem.h"
#include "private/themedlable.h"

#include <DFontSizeManager>
#include <DPushButton>
#include <DLabel>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLoggingCategory>

DWIDGET_USE_NAMESPACE
UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

WordWizardItem::WordWizardItem(DWidget *parent)
    : DWidget(parent)
{
    initUI();
}

WordWizardItem::~WordWizardItem()
{
}

void WordWizardItem::initUI()
{
    m_pLabelTheme = new DLabel;
    DFontSizeManager::instance()->bind(m_pLabelTheme, DFontSizeManager::T6, QFont::Medium);
    m_pLabelTheme->setElideMode(Qt::ElideRight);
    m_pLabelSummary = new DLabel;
    DFontSizeManager::instance()->bind(m_pLabelSummary, DFontSizeManager::T8, QFont::Normal);
    m_pLabelSummary->setElideMode(Qt::ElideRight);

    QVBoxLayout *textLayout = new QVBoxLayout;
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    textLayout->addWidget(m_pLabelTheme, 0, Qt::AlignLeft);
    textLayout->addWidget(m_pLabelSummary, 0, Qt::AlignLeft);

    m_pBtnSwitch = new DSwitchButton;
    QHBoxLayout *settingLayout = new QHBoxLayout;
    settingLayout->setContentsMargins(0, 0, 0, 0);
    settingLayout->addLayout(textLayout);
    settingLayout->addStretch();
    settingLayout->addWidget(m_pBtnSwitch, 0, Qt::AlignVCenter);
    connect(m_pBtnSwitch, &DSwitchButton::checkedChanged, this, &WordWizardItem::signalSwitchChanged);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 10, 10, 10);
    mainLayout->addLayout(settingLayout);

    setLayout(mainLayout);
}

void WordWizardItem::setText(const QString &theme, const QString &summary)
{
    m_pLabelTheme->setText(theme);
    m_pLabelTheme->setToolTip(theme);
    m_pLabelSummary->setText(summary);
    m_pLabelSummary->setToolTip(summary);
}

void WordWizardItem::setSwitchChecked(bool b)
{
    qCDebug(logAIGUI) << "Setting switch checked state:" << b;
    m_pBtnSwitch->setChecked(b);
}
