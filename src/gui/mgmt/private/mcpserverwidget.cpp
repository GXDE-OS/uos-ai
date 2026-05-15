#include "mcpserverwidget.h"
#include "themedlable.h"
#include "mcpserveritem.h"
#include "localmodelserver.h"
#include "mcpserverlistwidget.h"

#include <DLabel>
#include <DFontSizeManager>
#include <DBackgroundGroup>
#include <DGuiApplicationHelper>
#include <DPushButton>
#include <DCommandLinkButton>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLoggingCategory>
#include <QDir>

using namespace uos_ai;
DWIDGET_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

static constexpr int TIMER_BEGIN = 720; // 5秒轮询状态60分钟

McpServerWidget::McpServerWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    onThemeTypeChanged();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &McpServerWidget::onThemeTypeChanged);

    m_timer = new QTimer(this);
    m_timer->setInterval(5000);
    connect(m_timer, &QTimer::timeout, this, &McpServerWidget::checkStatusOntime);
    connect(&LocalModelServer::getInstance(), &LocalModelServer::beginCheck, this, [this](const QString &app, int count) {
        if (app != UOSAIAGENTNAME)
            return;

        beginTimer();
    });
}

McpServerWidget::~McpServerWidget()
{
    m_timer->stop();
}

void McpServerWidget::initUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(10);

    // 标题
    m_pWidgetLabel = new ThemedLable(getTitleName());
    m_pWidgetLabel->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_pWidgetLabel, DFontSizeManager::T5, QFont::Bold);
    m_mainLayout->addWidget(m_pWidgetLabel, 0, Qt::AlignLeft);

    // mcp环境标题
    m_pEnvWidgetLabel = new ThemedLable(tr("Mcp Enviorment"));
    m_pEnvWidgetLabel->setPaletteColor(QPalette::WindowText, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_pEnvWidgetLabel, DFontSizeManager::T6, QFont::Medium);
    m_mainLayout->addWidget(m_pEnvWidgetLabel);

    // 服务组件
    m_mainLayout->addWidget(serverWidget());
    m_mainLayout->addStretch();
}

void McpServerWidget::changeInstallStatus()
{
    if (m_pServerItem)
        m_pServerItem->changeInstallStatus(m_isInstalled);
#if 0
    if (m_isInstalled) {
        if (!m_pServersListWidget) {
            m_pServersListWidget = new McpServerListWidget(this);
            connect(this, &McpServerWidget::sigThirdPartyMcpAgree, m_pServersListWidget, &McpServerListWidget::refreshAllItemsCheckState);
            m_mainLayout->addWidget(m_pServersListWidget);
        }

        m_pServersListWidget->setVisible(true);
        m_pServersListWidget->updateMcpServersInfo();
    } else {
        if (m_pServersListWidget)
            m_pServersListWidget->setVisible(false);
    }
#endif
}

QString McpServerWidget::getTitleName()
{
    return tr("MCP Server");
}

void McpServerWidget::updateStatus()
{
    qCDebug(logAIGUI) << "Updating MCP server status";

    QDir dir("/var/lib/dpkg/info");
    bool newInstallStatus = !dir.entryList(QStringList() << QString("%1*.list").arg(UOSAIAGENTNAME)).isEmpty();

    if (m_isInstalled != newInstallStatus) {
        qCInfo(logAIGUI) << "MCP server install status changed. AppName:" << UOSAIAGENTNAME << ", Installed:" << newInstallStatus;
        m_isInstalled = newInstallStatus;
        emit LocalModelServer::getInstance().pluginStatusChanged(UOSAIAGENTNAME, m_isInstalled);
        changeInstallStatus();
        return;
    }

    if (m_pServerItem)
        m_pServerItem->checkUpdateStatus(m_isInstalled);
}

void McpServerWidget::beginTimer()
{
    qCDebug(logAIGUI) << "Begin timer for MCP server. Time:" << TIMER_BEGIN;
    m_timerCount = TIMER_BEGIN;
    m_timer->start();
}

void McpServerWidget::stopTimer()
{
    qCDebug(logAIGUI) << "Stop timer for MCP server.";
    m_timerCount = 0;
    m_timer->stop();
}

void McpServerWidget::onThemeTypeChanged()
{
    // 主题切换处理
    qCDebug(logAIGUI) << "MCP server widget theme changed";

    // 设置DBackgroundGroup的背景色
    if (m_pServerWidget) {
        DPalette pl = m_pServerWidget->palette();
        pl.setColor(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        m_pServerWidget->setPalette(pl);
    }
}

void McpServerWidget::checkStatusOntime()
{
    if (m_timerCount > 0) {
        updateStatus();
        --m_timerCount;
    } else {
        m_timerCount = 0;
        m_timer->stop();
    }
}

DBackgroundGroup *McpServerWidget::serverWidget()
{
    qCDebug(logAIGUI) << "Creating MCP server widget";

    m_pServerItem = new McpServerItem(this);
    m_pServerItem->setText(tr("Install UOS AI Agent"), 
                           tr("After installation, MCP Server will be available."));
    m_pServerItem->setAppName(UOSAIAGENTNAME);
    m_pServerItem->setMinimumHeight(60);
    m_pServerItem->setMaximumHeight(75);

    connect(m_pServerItem, &McpServerItem::doCheckInstalled, this, &McpServerWidget::beginTimer);

    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(m_pServerItem);

    m_pServerWidget = new DBackgroundGroup(bgLayout, this);
    m_pServerWidget->setContentsMargins(0, 0, 0, 0);

    return m_pServerWidget;
}
