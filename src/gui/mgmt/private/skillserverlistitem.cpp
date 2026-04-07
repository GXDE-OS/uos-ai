#include "skillserverlistitem.h"
#include "dconfigmanager.h"
#include "skillserverlistitemtooltip.h"
#include "dbwrapper.h"
#include "../common/echeckagreementdialog.h"

#include <DFontSizeManager>
#include <DGuiApplicationHelper>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLoggingCategory>
#include <QTimer>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

SkillServerListItem::SkillServerListItem(DWidget *parent)
    : DWidget(parent)
{
    m_pTooltipTimer = new QTimer(this);
    m_pTooltipTimer->setSingleShot(true);
    m_pTooltipTimer->setInterval(1000);

    initUI();
    initConnect();
    m_skills.reset(new SkillsManager);
}

SkillServerListItem::~SkillServerListItem()
{
    if (m_pCustomToolTip)
        m_pCustomToolTip->deleteLater();
}

void SkillServerListItem::initUI()
{
    // 主布局：左右结构
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 左侧区域：上下结构
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(5);

    // 左侧顶部：水平布局，包含服务名、内置标识、编辑按钮、删除按钮
    QHBoxLayout *topLeftLayout = new QHBoxLayout();
    topLeftLayout->setContentsMargins(0, 0, 0, 0);
    topLeftLayout->setSpacing(0);

    // 名称标签
    m_pNameLabel = new DLabel(this);
    DFontSizeManager::instance()->bind(m_pNameLabel, DFontSizeManager::T6, QFont::Medium);
    m_pNameLabel->setElideMode(Qt::ElideRight); // 设置文本省略
    m_pNameLabel->setMinimumWidth(10);
    m_pNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred); // 允许扩展

    // 内置标识标签
    m_pBuiltInLabel = new DLabel(tr("built-in"), this);
    DFontSizeManager::instance()->bind(m_pBuiltInLabel, DFontSizeManager::T10, QFont::Normal);
    // 更新样式表：添加背景色、描边和圆角
    m_pBuiltInLabel->setStyleSheet(
        "color: rgba(0, 130, 250, 1);"
        "background-color: rgba(0, 129, 255, 0.1);"
        "border: 1px solid rgba(0, 129, 255, 0.4);"
        "border-radius: 3px;"
        "padding: -1px 1px 0px;"
    );
    m_pBuiltInLabel->hide(); // 默认隐藏，只有内置服务才显示
    m_pBuiltInLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed); // 固定大小

    // 删除按钮
    m_pBtnDelete = new DIconButton(this);
    m_pBtnDelete->setIcon(QIcon::fromTheme("uos-ai-assistant_delete"));
    m_pBtnDelete->setFixedSize(24, 24);
    m_pBtnDelete->setIconSize(QSize(12, 14)); // 设置图标大小
    m_pBtnDelete->setFlat(true); // 设置为扁平样式
    m_pBtnDelete->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // 添加占位符来保持布局稳定
    m_pEditPlaceholder = new QWidget(this);
    m_pEditPlaceholder->setFixedSize(24, 24);
    m_pEditPlaceholder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pEditPlaceholder->hide();

    m_pDeletePlaceholder = new QWidget(this);
    m_pDeletePlaceholder->setFixedSize(24, 24);
    m_pDeletePlaceholder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pDeletePlaceholder->hide();

    // 将顶部左侧的组件添加到水平布局中
    topLeftLayout->addWidget(m_pNameLabel);
    topLeftLayout->addSpacing(5);
    topLeftLayout->addWidget(m_pBuiltInLabel);
    topLeftLayout->addWidget(m_pEditPlaceholder);
    topLeftLayout->addWidget(m_pBtnDelete);
    topLeftLayout->addWidget(m_pDeletePlaceholder);
    topLeftLayout->addStretch(10);

    // 左侧底部：描述标签
    m_pDescLabel = new DLabel(this);
    DFontSizeManager::instance()->bind(m_pDescLabel, DFontSizeManager::T8, QFont::Normal);
    m_pDescLabel->setElideMode(Qt::ElideRight);

    // 安装事件过滤器用于自定义tooltip
    m_pDescLabel->installEventFilter(this);

    // 将顶部和底部布局添加到左侧垂直布局
    leftLayout->addLayout(topLeftLayout);
    leftLayout->addWidget(m_pDescLabel);

    // 右侧区域：开关按钮
    m_pBtnSwitch = new DSwitchButton(this);
    // 安装事件过滤器用于拦截开关点击
    m_pBtnSwitch->installEventFilter(this);

    // 将左右区域添加到主布局
    mainLayout->addLayout(leftLayout);
    mainLayout->addStretch(); // 添加弹性空间，让开关靠右
    mainLayout->addWidget(m_pBtnSwitch);
    setLayout(mainLayout);
}

void SkillServerListItem::initConnect()
{
    connect(m_pBtnDelete, &DIconButton::clicked, this, &SkillServerListItem::onDelete);
    connect(m_pBtnSwitch, &DSwitchButton::checkedChanged, this, &SkillServerListItem::onSwitchChanged);
    connect(m_pTooltipTimer, &QTimer::timeout, this, &SkillServerListItem::showCustomToolTip);
}

void SkillServerListItem::setText(const QString &name, const QString &description)
{
    m_pNameLabel->setText(name);

    QString cleanedDescription = description;
    cleanedDescription.remove('\n');
    m_pDescLabel->setText(cleanedDescription);

    // 保存当前文本用于tooltip
    m_currentName = name;
    m_currentDescription = description;
}

void SkillServerListItem::showCustomToolTip()
{
    if (m_currentName.isEmpty() || m_currentDescription.isEmpty())
        return;
    
    if (!m_pCustomToolTip) {
        m_pCustomToolTip = new SkillServerListItemTooltip();
        m_pCustomToolTip->installEventFilter(this);
    }

    m_pCustomToolTip->show();
    m_pCustomToolTip->setContent(m_currentName, m_currentDescription);

    // 计算tooltip位置
    QPoint globalPos = m_pDescLabel->mapToGlobal(QPoint(0, 0));
    QSize labelSize = m_pDescLabel->size();
    QSize tooltipSize = m_pCustomToolTip->size();
    
    // 在描述标签下方显示tooltip
    QPoint tooltipPos(globalPos.x() + labelSize.width() / 2 - tooltipSize.width() / 2,
                      globalPos.y() + labelSize.height() + 5);
    
    m_pCustomToolTip->move(tooltipPos);
}

void SkillServerListItem::hideCustomToolTip(int delay)
{
    if (m_pTooltipTimer)
        m_pTooltipTimer->stop();

    if (m_pCustomToolTip)
        m_pCustomToolTip->hideTooltip(delay);
}

bool SkillServerListItem::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_pDescLabel) {
        if (event->type() == QEvent::Enter) {
            // 延迟显示tooltip，避免鼠标移动时频繁显示
            if (m_pTooltipTimer)
                m_pTooltipTimer->start();
        } else if (event->type() == QEvent::Leave) {
            hideCustomToolTip(300);
        } else if (event->type() == QEvent::MouseButtonPress) {
            hideCustomToolTip(0);
        }

        if (event->type() == QEvent::Wheel) {
            hideCustomToolTip(0);
        }
    }

    if (obj == m_pBtnSwitch) {
        if (!m_pBtnSwitch->isChecked() && event->type() == QEvent::MouseButtonPress) {
            // 在开关状态启动之前检查协议
            bool agreed = getThirdPartyMcpAgreement();
            if (!agreed) {
                // 如果用户未同意协议，阻止事件继续传播
                return true;
            } else {
                // 用户已同意协议，手动设置开关为开启状态并触发状态改变
                m_pBtnSwitch->setChecked(true);
                onSwitchChanged(true);
                // 阻止事件继续传播，避免重复触发
                return true;
            }
        }
    }

    if (m_pCustomToolTip && obj == m_pCustomToolTip) {
        if (event->type() == QEvent::Leave) {
            hideCustomToolTip(0);
        } else if (event->type() == QEvent::Enter) {
            if (m_pCustomToolTip)
                m_pCustomToolTip->holdTooltip();
        }
    }
    return DWidget::eventFilter(obj, event);
}

void SkillServerListItem::setSwitchChecked(bool checked)
{
    qCDebug(logAIGUI) << "Setting switch checked for SkillServerListItem. ServerId:" << m_serverId << ", Checked:" << checked;
    m_pBtnSwitch->setChecked(checked);
}

void SkillServerListItem::setBuiltIn(bool builtIn)
{
    m_isBuiltIn = builtIn;
    m_pBuiltInLabel->setVisible(m_isBuiltIn);
    updateButtonVisibility();
}

void SkillServerListItem::setServerId(const QString &serverId)
{
    m_serverId = serverId;

    if (m_pBtnSwitch && DbWrapper::localDbWrapper().getThirdPartyMcpAgreement()) {
        auto enabledSkills = m_skills->enabledSkills();

        // 创建已启用技能名称的集合
        QSet<QString> enabledSkillNames;
        for (const auto &skill : enabledSkills) {
            enabledSkillNames.insert(skill.name);
        }
        m_pBtnSwitch->setChecked(enabledSkillNames.contains(m_serverId));
    }
}

void SkillServerListItem::setDeletable(bool deletable)
{
    m_isDeletable = deletable;
    if (m_isDeletable) {
        m_pBtnDelete->setIcon(QIcon::fromTheme("uos-ai-assistant_delete"));
    } else {
        m_pBtnDelete->setIcon(QIcon::fromTheme("folder-symbolic"));
    }
}

QString SkillServerListItem::serverId()
{
    return m_serverId;
}

#ifdef COMPILE_ON_QT6
void SkillServerListItem::enterEvent(QEnterEvent *event)
#else
void SkillServerListItem::enterEvent(QEvent *event)
#endif
{
    m_isHovered = true;
    updateButtonVisibility();
    DWidget::enterEvent(event);
}

void SkillServerListItem::leaveEvent(QEvent *event)
{
    m_isHovered = false;
    updateButtonVisibility();
    DWidget::leaveEvent(event);
}

void SkillServerListItem::updateButtonVisibility()
{
    if (m_isBuiltIn) {
        // 内置服务不显示删除按钮
        m_pBtnDelete->hide();
        m_pEditPlaceholder->hide();
        m_pDeletePlaceholder->hide();
    } else {
        // 非内置服务，根据hover状态显示按钮
        if (m_isHovered) {
            m_pBtnDelete->show();
            m_pEditPlaceholder->hide();
            m_pDeletePlaceholder->hide();
        } else {
            m_pBtnDelete->hide();
            m_pEditPlaceholder->show();
            m_pDeletePlaceholder->show();
        }
    }
}

void SkillServerListItem::onDelete()
{
    qCInfo(logAIGUI) << "Delete button clicked. ServerId:" << m_serverId;
    emit signalDelete(m_serverId, m_isDeletable);
}

bool SkillServerListItem::getThirdPartyMcpAgreement()
{
    //先查询数据库，没同意就弹窗
    if(DbWrapper::localDbWrapper().getThirdPartyMcpAgreement())
        return true;

    ECheckAgreementDialog dlg;
    dlg.exec();

    bool agreed = DbWrapper::localDbWrapper().getThirdPartyMcpAgreement();
    if (agreed) {
        // 同意协议时，通知刷新所有SkillServerListItem的check状态
        emit signalAgreementAccepted();
    }
    return agreed;
}


void SkillServerListItem::onSwitchChanged(bool checked)
{
    m_skills->setSkillEnabled(m_serverId, checked);
}
