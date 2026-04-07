#include "skillserverlistwidget.h"
#include "skillserverlistitem.h"
#include "custommcpservereditor.h"
#include "dconfigmanager.h"
#include "dbwrapper.h"
#include "../common/echeckagreementdialog.h"
#include "global_define.h"
#include "mcpconfigsyncer.h"

#include "agentfactory.h"

#include <DFontSizeManager>
#include <DDialog>
#include <DFileDialog>
#include <DFileIconProvider>
#include <QtConcurrent>
#include <QDesktopServices>
#include <docparser.h>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

SkillServerListWidget::SkillServerListWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &SkillServerListWidget::onThemeTypeChanged);
    connect(McpConfigSyncer::instance(), &McpConfigSyncer::asyncFetchSuccess, this, [ = ]{
        updateSkillServersInfo();
    });
    m_lastImportPath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
    m_skillsPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.uos-ai/skills/";
    m_supSuffix << "SKILL.md";
    m_skills.reset(new SkillsManager);
    connect(m_skills.data(), &SkillsManager::skillsUpdated, this, [this]() {
        updateSkillServersInfo();
    });
}

void SkillServerListWidget::updateSkillServersInfo()
{
    if (m_skills.isNull())
        m_skills.reset(new SkillsManager);
    m_pSkillInfo = m_skills->skills();
    updateServerList();
}

void SkillServerListWidget::onThemeTypeChanged()
{
    // 设置DBackgroundGroup的背景色
    for (auto item : m_builtInItem) {
        DPalette pl = item->palette();
        pl.setColor(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        item->setPalette(pl);
    }

    for (auto item : m_thirdBuiltInItem) {
        DPalette pl = item->palette();
        pl.setColor(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        item->setPalette(pl);
    }

    for (auto item : m_customItem) {
        DPalette pl = item->palette();
        pl.setColor(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        item->setPalette(pl);
    }
}

void SkillServerListWidget::initUI()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);

    // 顶部标题栏
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);

    // 标题标签
    m_pTitleLabel = new DLabel(tr("Skill List"), this);
    DFontSizeManager::instance()->bind(m_pTitleLabel, DFontSizeManager::T6, QFont::Medium);
    topLayout->addWidget(m_pTitleLabel);

    topLayout->addStretch();

    // 刷新按钮
    m_pReloadButton = new DCommandLinkButton(tr("Refresh"), this);
    DFontSizeManager::instance()->bind(m_pReloadButton, DFontSizeManager::T8, QFont::Normal);
    topLayout->addWidget(m_pReloadButton);

    // 添加按钮
    m_pAddButton = new DCommandLinkButton(tr("Import Skill"), this);
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
    connect(m_pAddButton, &DPushButton::clicked,
            this, &SkillServerListWidget::onAddServerClicked);
    connect(m_pReloadButton, &DPushButton::clicked,
            this, [this](){
        m_skills->setupFileWatcher();
        m_skills->reloadSkills();
    });

    setLayout(mainLayout);
}

void SkillServerListWidget::onAddServerClicked()
{
    if (!getThirdPartyMcpAgreement()) {
        qCWarning(logAIGUI) << "User has not accepted the third-party MCP agreement.";
        return;
    }

    if (m_skills.isNull())
        m_skills.reset(new SkillsManager);

    DFileDialog fileDlg;
    fileDlg.setDirectory(m_lastImportPath);
    QString selfilter = tr("Supported files") + (" (%1)");
    selfilter = selfilter.arg("SKILL.md *.skill *.zip *.tar.gz *.tgz *.tar.bz2 *.tbz2 *.tar.xz *.txz");
    fileDlg.setViewMode(DFileDialog::Detail);
    fileDlg.setFileMode(DFileDialog::ExistingFile);
    fileDlg.setNameFilter(selfilter);
    fileDlg.selectNameFilter(selfilter);
    fileDlg.setObjectName("fileDialogAdd");
    bool result = fileDlg.exec();
    if (DFileDialog::Accepted == result) {
        m_lastImportPath = fileDlg.directory().path();
        QStringList selectedPaths = fileDlg.selectedFiles();
        if (selectedPaths.isEmpty()) {
            qCWarning(logAIGUI) << "No file selected.";
            return;
        }

        // 文件选择框中创建新文件不会过滤，需要再判断一次
        QFileInfo docInfo(selectedPaths[0]);
        bool isSkillMd = (docInfo.fileName() == QLatin1String("SKILL.md"));
        QString lowerSuffix = docInfo.suffix().toLower();
        QString lowerBase = docInfo.completeBaseName().toLower();
        bool isArchive = (lowerSuffix == "zip" || lowerSuffix == "tgz" ||
                          lowerSuffix == "tbz2" || lowerSuffix == "txz" ||
                          lowerSuffix == "skill" ||
                          (lowerSuffix == "gz" && lowerBase.endsWith(".tar")) ||
                          (lowerSuffix == "bz2" && lowerBase.endsWith(".tar")) ||
                          (lowerSuffix == "xz" && lowerBase.endsWith(".tar")));
        if (!isSkillMd && !isArchive) {
            qCWarning(logAIGUI) << "Unsupported file format:" << docInfo.fileName();
            return;
        }

        QString errorMsg;
        QString skillName;
        if (!m_skills->addSkill(selectedPaths[0], &errorMsg, &skillName)) {
            DDialog dlg(this);
            dlg.setMinimumWidth(380);
            dlg.setIcon(QIcon(":/assets/images/warning.svg"));
            dlg.setTitle(tr("Import Failed"));
            dlg.setMessage(errorMsg.isEmpty() ? tr("Failed to import the skill.") : errorMsg);
            dlg.addButton(tr("OK"), true, DDialog::ButtonNormal);
            dlg.exec();
            return;
        }

        DDialog dlg(this);
        dlg.setMinimumWidth(380);
        dlg.setIcon(QIcon(":/assets/images/ok_info.svg"));
        dlg.setTitle(tr("Import Successful"));
        dlg.setMessage(tr("%1 Import Successful").arg(skillName));
        dlg.addButton(tr("OK"), true, DDialog::ButtonRecommend);
        dlg.exec();

        updateSkillServersInfo();
    }
}

void SkillServerListWidget::removeCustomSkillServer(const QString &name, bool isDeletable)
{
    if (m_skills.isNull())
        m_skills.reset(new SkillsManager);

    if (!isDeletable) {
        QString skillPath = m_skills->readSkill(name).path;
        QDir dir(skillPath);
        if (dir.exists()) {
            // 使用 QDesktopServices 打开目录
            QDesktopServices::openUrl(QUrl::fromLocalFile(skillPath));
        }
        return;
    }

    if (!showRmSkillServerDlg(name))
        return;

    if (!m_skills->removeSkill(name)) {
        return;
    }
    updateSkillServersInfo();
}

void SkillServerListWidget::resetSkillServerItems()
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

void SkillServerListWidget::updateServerList()
{
    resetSkillServerItems();

    if (m_pSkillInfo.isEmpty())
        return;

    // 遍历所有技能信息，读取名称并创建对应的服务器项
    for (auto &skillInfo : m_pSkillInfo) {
        if (skillInfo.name.isEmpty()) {
            qWarning() << "Skipping skill with empty name";
            continue;
        }

        DBackgroundGroup *item = creatServerItem(skillInfo);
        if (!item) {
            qWarning() << "Failed to create server item for skill:" << skillInfo.name;
            continue;
        }

        QString skillName = skillInfo.name;
        m_customItem.append(item);
        qDebug() << "Added skill to server list:" << skillName;
    }

    // 内置SKILl显示最前
    for (auto item : m_builtInItem)
        m_pListLayout->addWidget(item);

    // 内置三方SKILl显示在中
    for (auto item : m_thirdBuiltInItem)
        m_pListLayout->addWidget(item);

    // 自定义SKILl显示在后
    for (auto item : m_customItem)
        m_pListLayout->addWidget(item);

    qDebug() << "Server list updated. Total skills:" << m_customItem.size();
}

DBackgroundGroup *SkillServerListWidget::creatServerItem(SkillInfo &info)
{
    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);

    // 将SkillServerListItem包装在DBackgroundGroup中
    DBackgroundGroup *backgroundGroup = new DBackgroundGroup(bgLayout, this);
    backgroundGroup->setContentsMargins(0, 0, 0, 0);
    DPalette pl = backgroundGroup->palette();
    pl.setColor(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    backgroundGroup->setPalette(pl);

    QString name = info.name;
    QString description = info.description;
    bool isBuiltIn = (info.source == "builtin");

    // 创建SkillServerListItem
    SkillServerListItem *listItem = new SkillServerListItem(backgroundGroup);
    listItem->setText(name, description);
    listItem->setBuiltIn(isBuiltIn);
    listItem->setServerId(name);
    listItem->setDeletable(info.source == "uos-ai");
    bgLayout->addWidget(listItem);

    // 连接信号槽
    connect(listItem, &SkillServerListItem::signalDelete, this, &SkillServerListWidget::removeCustomSkillServer);
    connect(listItem, &SkillServerListItem::signalAgreementAccepted, this, &SkillServerListWidget::refreshAllItemsCheckState);

    return backgroundGroup;
}

bool SkillServerListWidget::showRmSkillServerDlg(const QString &name)
{
    DDialog dlg(this);
    dlg.setMinimumWidth(380);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.setTitle(QString(tr("Confirm deletion %1?")).arg(name));
    dlg.setMessage(tr("After deletion, this skill will be unavailable. Proceed with caution."));
    dlg.addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    dlg.addButton(tr("Delete"), true, DDialog::ButtonWarning);
    if (dlg.exec() == DDialog::Accepted)
        return true;

    return false;
}

bool SkillServerListWidget::getThirdPartyMcpAgreement()
{
    //先查询数据库，没同意就弹窗
    if(DbWrapper::localDbWrapper().getThirdPartyMcpAgreement())
        return true;

    ECheckAgreementDialog dlg;
    dlg.exec();

    bool agreed = DbWrapper::localDbWrapper().getThirdPartyMcpAgreement();
    if (agreed) {
        // 同意协议时，刷新所有SkillServerListItem的check状态
        refreshAllItemsCheckState();
    }
    return agreed;
}

void SkillServerListWidget::refreshAllItemsCheckState()
{
    if (m_skills.isNull())
        m_skills.reset(new SkillsManager);
    
    // 获取当前启用的技能列表
    auto enabledSkills = m_skills->enabledSkills();
    
    // 创建已启用技能名称的集合
    QSet<QString> enabledSkillNames;
    for (const auto &skill : enabledSkills) {
        enabledSkillNames.insert(skill.name);
    }

    // 刷新所有内置服务器项的check状态
    for (auto bgGroup : m_builtInItem) {
        SkillServerListItem *listItem = qobject_cast<SkillServerListItem*>(bgGroup->layout()->itemAt(0)->widget());
        if (listItem) {
            listItem->setSwitchChecked(enabledSkillNames.contains(listItem->serverId()));
        }
    }

    // 刷新所有内置三方服务器项的check状态
    for (auto bgGroup : m_thirdBuiltInItem) {
        SkillServerListItem *listItem = qobject_cast<SkillServerListItem*>(bgGroup->layout()->itemAt(0)->widget());
        if (listItem) {
            listItem->setSwitchChecked(enabledSkillNames.contains(listItem->serverId()));
        }
    }

    // 刷新所有自定义服务器项的check状态
    for (auto bgGroup : m_customItem) {
        SkillServerListItem *listItem = qobject_cast<SkillServerListItem*>(bgGroup->layout()->itemAt(0)->widget());
        if (listItem) {
            listItem->setSwitchChecked(enabledSkillNames.contains(listItem->serverId()));
        }
    }
}
