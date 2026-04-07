#include "skillserverwidget.h"
#include "themedlable.h"
#include "skillserveritem.h"
#include "localmodelserver.h"
#include "dconfigmanager.h"
#include "skillserverlistwidget.h"

#include <DLabel>
#include <DFontSizeManager>
#include <DBackgroundGroup>
#include <DGuiApplicationHelper>
#include <DPushButton>
#include <DCommandLinkButton>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLoggingCategory>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

SkillServerWidget::SkillServerWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    onThemeTypeChanged();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &SkillServerWidget::onThemeTypeChanged);
}

SkillServerWidget::~SkillServerWidget()
{
}

void SkillServerWidget::initUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(10);

    // 标题
    m_pWidgetLabel = new ThemedLable(getTitleName());
    m_pWidgetLabel->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_pWidgetLabel, DFontSizeManager::T5, QFont::Bold);
    m_mainLayout->addWidget(m_pWidgetLabel, 0, Qt::AlignLeft);

    // skill环境标题
    m_pEnvWidgetLabel = new ThemedLable(tr("Skill Enviorment"));
    m_pEnvWidgetLabel->setPaletteColor(QPalette::WindowText, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_pEnvWidgetLabel, DFontSizeManager::T6, QFont::Medium);
    m_mainLayout->addWidget(m_pEnvWidgetLabel);

    // 服务组件
    m_mainLayout->addWidget(serverWidget());
    m_mainLayout->addStretch();
}

void SkillServerWidget::changeInstallStatus(bool isInstall)
{
    m_isInstalled = isInstall;
    if (m_pServerItem)
        m_pServerItem->changeInstallStatus(m_isInstalled);

    if (m_isInstalled) {
        if (!m_pServersListWidget) {
            m_pServersListWidget = new SkillServerListWidget(this);
            connect(this, &SkillServerWidget::sigThirdPartyMcpAgree, m_pServersListWidget, &SkillServerListWidget::refreshAllItemsCheckState);
            m_mainLayout->addWidget(m_pServersListWidget);
        }
        m_pServersListWidget->setVisible(true);
        m_pServersListWidget->updateSkillServersInfo();
        m_pEnvWidgetLabel->setVisible(false);
        m_pServerWidget->setVisible(false);
    } else {
        if (m_pServersListWidget)
            m_pServersListWidget->setVisible(false);
        m_pEnvWidgetLabel->setVisible(true);
        m_pServerWidget->setVisible(true);
    }
}

QString SkillServerWidget::getTitleName()
{
    return tr("Agent Skills");
}

void SkillServerWidget::updateStatus()
{
    qCDebug(logAIGUI) << "Updating Skill server status";

    changeInstallStatus(m_isInstalled);

    if (m_pServerItem)
        m_pServerItem->checkUpdateStatus(m_isInstalled);
}

void SkillServerWidget::onThemeTypeChanged()
{
    // 主题切换处理
    qCDebug(logAIGUI) << "Skill server widget theme changed";

    // 设置DBackgroundGroup的背景色
    if (m_pServerWidget) {
        DPalette pl = m_pServerWidget->palette();
        pl.setColor(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        m_pServerWidget->setPalette(pl);
    }
    if (m_pServerItem) {
        m_pServerItem->setText(tr("Configure "),
                               tr("MCP & Skills environment"),
                               tr("After configuration you can use agent skills"));
    }
}

DBackgroundGroup *SkillServerWidget::serverWidget()
{
    qCDebug(logAIGUI) << "Creating skill server widget";

    m_pServerItem = new SkillServerItem(this);
    m_pServerItem->setText(tr("Configure "),
                           tr("MCP & Skills environment"),
                           tr("After configuration you can use agent skills"));
    connect(m_pServerItem, SIGNAL(sigNavigateToMcpServer()), this, SIGNAL(sigNavigateToMcpServerPage()));
    m_pServerItem->setMinimumHeight(60);
    m_pServerItem->setMaximumHeight(75);

    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(m_pServerItem);

    m_pServerWidget = new DBackgroundGroup(bgLayout, this);
    m_pServerWidget->setContentsMargins(0, 0, 0, 0);

    return m_pServerWidget;
}
