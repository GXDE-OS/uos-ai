#include "skillserveritem.h"
#include "localmodelserver.h"

#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DDialog>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLoggingCategory>
#include <QtConcurrent>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

SkillServerItem::SkillServerItem(DWidget *parent)
    : DWidget(parent)
{
    initUI();
}

SkillServerItem::~SkillServerItem()
{
}

void SkillServerItem::initUI()
{
    // 左侧信息区域
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(2);

    m_pNameLabel = new DLabel();
    DFontSizeManager::instance()->bind(m_pNameLabel, DFontSizeManager::T6, QFont::Medium);
    // 设置标题为超链接样式
    m_pNameLabel->setTextFormat(Qt::RichText);
    m_pNameLabel->setOpenExternalLinks(false);
    m_pNameLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_pNameLabel->setFocusPolicy(Qt::NoFocus);
    // 连接链接点击信号
    connect(m_pNameLabel, &DLabel::linkActivated, this, [this](const QString &link) {
        qCDebug(logAIGUI) << "Link activated:" << link;
        if (link == "http://mcp.server") {
            qCDebug(logAIGUI) << "Emitting sigNavigateToMcpServer signal";
            emit sigNavigateToMcpServer();
        }
    });

    m_pDescLabel = new DLabel();
    DFontSizeManager::instance()->bind(m_pDescLabel, DFontSizeManager::T8, QFont::Normal);
    m_pDescLabel->setElideMode(Qt::ElideRight);

    infoLayout->addWidget(m_pNameLabel);
    infoLayout->addWidget(m_pDescLabel);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addLayout(infoLayout, 10);
    mainLayout->setSpacing(10);
    mainLayout->addStretch();

    // 设置鼠标悬停样式
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
}

void SkillServerItem::setText(const QString &theme, const QString &target, const QString &summary)
{
    // 设置标题为超链接样式
    QString activeColor = DGuiApplicationHelper::instance()->applicationPalette()
                          .color(DPalette::Normal, DPalette::Highlight)
                          .name(QColor::HexRgb);

    QString linkText = QString("%1<a href=\"http://mcp.server\" style=\"text-decoration: none; color: %2;\">%3</a>")
                          .arg(theme)
                          .arg(activeColor)
                          .arg(target);
    m_pNameLabel->setText(linkText);
    
    m_pDescLabel->setText(summary);
    m_pDescLabel->setToolTip(summary);
}

void SkillServerItem::checkUpdateStatus(bool isInstalled)
{
    if (!isInstalled) {
        adjustSummaryLabelWidth();
        return;
    }
}

void SkillServerItem::adjustSummaryLabelWidth()
{
    int maxWidth = this->size().width() - 40;
    m_pDescLabel->setMaximumWidth(maxWidth < 0 ? 0 : maxWidth);
}

void SkillServerItem::changeInstallStatus(bool isInstalled)
{
    checkUpdateStatus(isInstalled);
    adjustSummaryLabelWidth();
}

void SkillServerItem::resizeEvent(QResizeEvent *event)
{
    DWidget::resizeEvent(event);
    adjustSummaryLabelWidth();
}
