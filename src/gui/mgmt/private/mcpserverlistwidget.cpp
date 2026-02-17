#include "mcpserverlistwidget.h"
#include "mcpserverlistitem.h"
#include "custommcpservereditor.h"
#include "dbwrapper.h"
#include "../common/echeckagreementdialog.h"
#include "global_define.h"
#include "mcpconfigsyncer.h"

#include "agentfactory.h"
#include "mcpserver.h"

#include <DFontSizeManager>
#include <DDialog>

using namespace uos_ai;

McpFilterComboBox::McpFilterComboBox(DWidget *parent)
    : DComboBox(parent)
{
}

void McpFilterComboBox::showPopup()
{
    if (!m_pMenu) {
        m_pMenu = new DMenu(this);

        // 获取当前所有可选项
        for (int i = 0; i < count(); ++i) {
            QString text = itemText(i);
            QAction *action = new QAction(text, m_pMenu);
            action->setData(i);  // 存储索引
            action->setCheckable(true);
            m_pMenu->addAction(action);
        }

        // 连接菜单项点击信号
        connect(m_pMenu, &DMenu::triggered, this, [this](QAction *action) {
            int index = action->data().toInt();
            if (index >= 0 && index < count())
                setCurrentIndex(index);
        });
    }

    for (auto action : m_pMenu->actions()) {
        int index = action->data().toInt();
        action->setChecked(currentIndex() == index);
    }

    // 显示菜单在合适的位置
    QPoint pos = mapToGlobal(QPoint(width() / 2, height()));
    m_pMenu->exec(pos);
}

void McpFilterComboBox::mousePressEvent(QMouseEvent *e)
{
    showPopup();
}

McpServerListWidget::McpServerListWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &McpServerListWidget::onThemeTypeChanged);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, &McpServerListWidget::onFontChanged);
    connect(McpConfigSyncer::instance(), &McpConfigSyncer::asyncFetchSuccess, this, [ = ]{
        updateMcpServersInfo();
    });
}

void McpServerListWidget::updateMcpServersInfo()
{
    auto mcpServer = AgentFactory::instance()->getMCPServer(kDefaultAgentName);
    if (!mcpServer)
        return;

    mcpServer->scanServers();
    auto sers = mcpServer->serverNames();
    QJsonArray sersArray {};
    for (auto it = sers.begin(); it != sers.end(); ++it) {
        QJsonObject obj({{"name", *it}, {"builtin", mcpServer->isBuiltin(*it)}, {"removable", mcpServer->isRemovable(*it)},
                         {"description", mcpServer->description(*it)}});
        sersArray.append(obj);
    }

    m_pServersInfo = sersArray;
    updateServerList();
}

void McpServerListWidget::onThemeTypeChanged()
{
    // 设置DBackgroundGroup的背景色
    for (auto it = m_builtInItem.begin(); it != m_builtInItem.end(); ++it) {
        DPalette pl = it.value()->palette();
        pl.setColor(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        it.value()->setPalette(pl);
    }

    for (auto it = m_thirdBuiltInItem.begin(); it != m_thirdBuiltInItem.end(); ++it) {
        DPalette pl = it.value()->palette();
        pl.setColor(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        it.value()->setPalette(pl);
    }

    for (auto it = m_customItem.begin(); it != m_customItem.end(); ++it) {
        DPalette pl = it.value()->palette();
        pl.setColor(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        it.value()->setPalette(pl);
    }
}

void McpServerListWidget::initUI()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);

    // 顶部标题栏
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(10);

    // 标题标签
    m_pTitleLabel = new DLabel(tr("MCP Server"), this);
    DFontSizeManager::instance()->bind(m_pTitleLabel, DFontSizeManager::T6, QFont::Medium);
    topLayout->addWidget(m_pTitleLabel);

    // 过滤下拉框
    m_pFilterComboBox = new McpFilterComboBox(this);
    m_pFilterComboBox->setFocusPolicy(Qt::NoFocus);
    m_pFilterComboBox->addItem(tr("All"));
    m_pFilterComboBox->addItem(tr("Built-in"));
    m_pFilterComboBox->addItem(tr("Custom"));
    m_pFilterComboBox->setFixedHeight(30);
    updateFilterWidth();
    topLayout->addWidget(m_pFilterComboBox);

    topLayout->addStretch(1);

    // 添加按钮
    m_pAddButton = new DCommandLinkButton(tr("Add MCP Server"), this);
    DFontSizeManager::instance()->bind(m_pAddButton, DFontSizeManager::T8, QFont::Normal);
    topLayout->addWidget(m_pAddButton);

    mainLayout->addLayout(topLayout);

    // 列表区域
    m_pListWidget = new DWidget(this);
    m_pListLayout = new QVBoxLayout(m_pListWidget);
    m_pListLayout->setContentsMargins(0, 0, 0, 0);
    m_pListLayout->setSpacing(10);
    m_pListLayout->addStretch();

    mainLayout->addWidget(m_pListWidget);

    // 连接信号槽
    connect(m_pFilterComboBox, QOverload<int>::of(&DComboBox::currentIndexChanged),
            this, &McpServerListWidget::onFilterChanged);
    connect(m_pAddButton, &DPushButton::clicked,
            this, &McpServerListWidget::onAddServerClicked);

    setLayout(mainLayout);
}

void McpServerListWidget::onFilterChanged(int index)
{
    m_currentFilter = index;
    updateFilterWidth();
    showItemsByFilter();
}

void McpServerListWidget::onAddServerClicked()
{
    if (!getThirdPartyMcpAgreement())
        return;

    CustomMcpServerEditor *editor = new CustomMcpServerEditor();

    if (editor->showAddMode())
        updateMcpServersInfo();

    editor->deleteLater();
}

void McpServerListWidget::onEditServerClicked(const QString &name)
{
    auto mcpServer = AgentFactory::instance()->getMCPServer(kDefaultAgentName);
    if (!mcpServer)
        return;

    mcpServer->scanServers();
    QJsonObject config = mcpServer->serverConfig(name);
    QString desc = mcpServer->description(name);

    CustomMcpServerEditor *editor = new CustomMcpServerEditor();

    if (editor->showEditMode(config, desc))
        updateMcpServersInfo();

    editor->deleteLater();
}

void McpServerListWidget::removeCustomMcpServer(const QString &name)
{
    if (!showRmMcpServerDlg(name))
        return;

    QSharedPointer<MCPServer> mcpServer = AgentFactory::instance()->getMCPServer(kDefaultAgentName);
    if (!mcpServer)
        return;

    mcpServer->scanServers();
    QPair<bool, QString> res = mcpServer->removeCustomServer(name);
    if (!res.first)
        return;

    updateMcpServersInfo();
}

void McpServerListWidget::onFontChanged()
{
    updateFilterWidth();
}

void McpServerListWidget::resetMcpServerItems()
{
    // 清空现有项目
    QLayoutItem *child;
    while ((child = m_pListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->setParent(nullptr);
            delete child->widget();
        }
        delete child;
    }
    m_thirdBuiltInItem.clear();
    m_builtInItem.clear();
    m_customItem.clear();
}

void McpServerListWidget::updateServerList()
{
    resetMcpServerItems();

    if (m_pServersInfo.isEmpty())
        return;

    for (const auto &serverInfo : m_pServersInfo) {
        QJsonObject obj = serverInfo.toObject();
        bool isBuiltin = obj.value("builtin").toBool();

        DBackgroundGroup *item = creatServerItem(serverInfo);
        if (!item)
            continue;
        QString serverName = obj.value("name").toString();

        if (isBuiltin) {
            // 添加内置服务器项
            if (serverName.compare("uos-mcp", Qt::CaseInsensitive) == 0) {
                m_builtInItem[serverName] = item;
            } else {
                m_thirdBuiltInItem[serverName] = item;
            }
        } else {
            // 添加自定义服务器项
            m_customItem[serverName] = item;
        }
    }

    // 内置服务器显示最前
    for (auto item : m_builtInItem)
        m_pListLayout->addWidget(item);

    // 内置三方服务器显示在中
    for (auto item : m_thirdBuiltInItem)
        m_pListLayout->addWidget(item);

    // 自定义三方服务器显示在后
    for (auto item : m_customItem)
        m_pListLayout->addWidget(item);

    showItemsByFilter();
    m_pFilterComboBox->setVisible(!m_customItem.isEmpty());
}

DBackgroundGroup *McpServerListWidget::creatServerItem(const QJsonValue &info)
{
    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);

    // 将McpServerListItem包装在DBackgroundGroup中
    DBackgroundGroup *backgroundGroup = new DBackgroundGroup(bgLayout, this);
    backgroundGroup->setContentsMargins(0, 0, 0, 0);
    DPalette pl = backgroundGroup->palette();
    pl.setColor(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    backgroundGroup->setPalette(pl);

    QJsonObject obj = info.toObject();
    QString name = obj.value("name").toString();
    QString description = obj.value("description").toString();
    bool isBuiltin = obj.value("builtin").toBool();

    // 创建McpServerListItem
    McpServerListItem *listItem = new McpServerListItem(backgroundGroup);
    listItem->setText(name, description);
    listItem->setBuiltIn(isBuiltin);
    listItem->setServerId(name);
    bgLayout->addWidget(listItem);

    // 连接信号槽
    connect(listItem, &McpServerListItem::signalEdit, this, &McpServerListWidget::onEditServerClicked);
    connect(listItem, &McpServerListItem::signalDelete, this, &McpServerListWidget::removeCustomMcpServer);

    return backgroundGroup;
}

bool McpServerListWidget::showRmMcpServerDlg(const QString &name)
{
    DDialog dlg(this);
    dlg.setMinimumWidth(380);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.setTitle(QString(tr("Confirm deletion %1?")).arg(name));
    dlg.setMessage(tr("After deletion, this server will be unavailable. Proceed with caution."));
    dlg.addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    dlg.addButton(tr("Delete"), true, DDialog::ButtonWarning);
    if (dlg.exec() == DDialog::Accepted)
        return true;

    return false;
}

bool McpServerListWidget::getThirdPartyMcpAgreement()
{
    //先查询数据库，没同意就弹窗
    if(DbWrapper::localDbWrapper().getThirdPartyMcpAgreement())
        return true;

    ECheckAgreementDialog dlg;
    dlg.exec();

    return DbWrapper::localDbWrapper().getThirdPartyMcpAgreement();
}

void McpServerListWidget::updateFilterWidth()
{
    QString text = m_pFilterComboBox->currentText();
    QFontMetrics fontMetrics(font());
    int textWidth = fontMetrics.horizontalAdvance(text);

    // 获取当前的最小和最大宽度限制
    int minWidth = 95;
    int maxWidth = 166;

    // 确保宽度在最小和最大范围内
    int newWidth = qMax(minWidth, qMin(maxWidth, textWidth + 50));

    // 设置固定宽度
    m_pFilterComboBox->setFixedWidth(newWidth);
    m_pFilterComboBox->setToolTip((textWidth + 50) > maxWidth ? text : "");
}

void McpServerListWidget::showItemsByFilter()
{
    // 内置服务器显示
    for (auto item : m_builtInItem)
        item->setVisible(m_customItem.isEmpty() || m_currentFilter != 2);

    // 内置三方服务器显示
    for (auto item : m_thirdBuiltInItem)
        item->setVisible(m_customItem.isEmpty() || m_currentFilter != 2);

    // 自定义三方服务器显示
    for (auto item : m_customItem)
        item->setVisible(!m_customItem.isEmpty() && m_currentFilter != 1);
}
