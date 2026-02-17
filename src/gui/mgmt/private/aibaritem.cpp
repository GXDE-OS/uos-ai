#include "aibaritem.h"

#include <DFontSizeManager>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLoggingCategory>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

AiBarItem::AiBarItem(DWidget *parent)
    : DWidget(parent)
{
    initUI();
}

AiBarItem::~AiBarItem()
{
}

void AiBarItem::initUI()
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
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addLayout(textLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(m_pBtnSwitch, 0, Qt::AlignVCenter);
    connect(m_pBtnSwitch, &DSwitchButton::checkedChanged, this, &AiBarItem::signalSwitchChanged);

    setLayout(mainLayout);
}

void AiBarItem::setText(const QString &theme, const QString &summary)
{
    m_pLabelTheme->setText(theme);
    m_pLabelSummary->setText(summary);
}

void AiBarItem::setSwitchChecked(bool b)
{
    qCDebug(logAIGUI) << "Setting switch checked state to" << b;
    m_pBtnSwitch->setChecked(b);
}


