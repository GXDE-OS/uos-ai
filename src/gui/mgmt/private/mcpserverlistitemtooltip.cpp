#include "mcpserverlistitemtooltip.h"

#include <DFontSizeManager>
#include <DPlatformWindowHandle>

#include <QDesktopServices>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

McpServerListItemTooltip::McpServerListItemTooltip(QWidget *parent)
    : DWidget(parent)
{
    initUI();

    m_pHideTimer = new QTimer(this);
    m_pHideTimer->setSingleShot(true);
    connect(m_pHideTimer, &QTimer::timeout, this, &McpServerListItemTooltip::hide);
}

void McpServerListItemTooltip::setContent(const QString &name, const QString &description)
{
    QString htmlContent;

    // 获取系统字号设置
    int titleFontSize = DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T6);
    int contentFontSize = DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T8);
    
    // 服务名称 - 使用较大的字体
    htmlContent += QString("<div style=\"font-size: %1px; font-weight: 500; margin-bottom: 8px;\">%2</div>")
                   .arg(titleFontSize)
                   .arg(name.toHtmlEscaped());

    // 处理描述文本，检测并转换链接
    QString processedDescription = description;
    
    // 使用正则表达式检测URL
    QRegularExpression urlRegex(
        R"((https?://[^\s<>"']+|www\.[^\s<>"']+))",
        QRegularExpression::CaseInsensitiveOption
    );
    
    QRegularExpressionMatchIterator i = urlRegex.globalMatch(description);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString url = match.captured(1);
        QString displayUrl = url;
        
        // 创建HTML链接
        QString htmlLink = QString("<a href=\"%1\" style=\"color: #2CA7F8; text-decoration: none;\">%2</a>")
                          .arg(url.toHtmlEscaped())
                          .arg(displayUrl.toHtmlEscaped());
        
        // 替换原始文本中的URL为HTML链接
        processedDescription.replace(match.captured(1), htmlLink);
    }

    // 将换行符转换为HTML换行标签
    processedDescription.replace("\n", "<br>");
    
    // 描述 - 使用较小的字体，支持链接
    htmlContent += QString("<div style=\"font-size: %1px; font-weight: 400;\">%2</div>")
                   .arg(contentFontSize)
                   .arg(processedDescription);

    // 使用QTextBrowser显示HTML内容，支持链接高亮和点击
    m_pTextBrowser->setHtml(htmlContent);
    
    // 调整大小
    m_pTextBrowser->document()->setDocumentMargin(10);
    QSize docSize = m_pTextBrowser->document()->size().toSize();
    
    // 设置QTextBrowser的高度
    int textBrowserHeight = docSize.height();
    m_pTextBrowser->setFixedHeight(textBrowserHeight);
    
    // 设置tooltip的固定高度
    setFixedHeight(textBrowserHeight);
}

void McpServerListItemTooltip::hideTooltip(int delay)
{
    if (delay > 0) {
        m_pHideTimer->start(delay);
    } else {
        m_pHideTimer->stop();
        hide();
    }
}

void McpServerListItemTooltip::holdTooltip()
{
    m_pHideTimer->stop();
}

void McpServerListItemTooltip::onAnchorClicked(const QUrl &url)
{
    QDesktopServices::openUrl(url);
    hideTooltip(0);
}

void McpServerListItemTooltip::initUI()
{
    setFixedWidth(354);
    setWindowFlags(Qt::ToolTip);
    setAutoFillBackground(true);

    DPlatformWindowHandle handle(this, this);
    handle.setWindowRadius(8);

    m_pLayout = new QVBoxLayout(this);
    m_pLayout->setContentsMargins(0, 0, 0, 0);

    // 创建QTextBrowser用于显示富文本内容，支持链接
    m_pTextBrowser = new QTextBrowser(this);
    m_pTextBrowser->setMaximumWidth(354);
    m_pTextBrowser->setFrameStyle(QFrame::NoFrame);
    m_pTextBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
    m_pTextBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
    m_pTextBrowser->setAttribute(Qt::WA_TranslucentBackground);
    m_pTextBrowser->setContentsMargins(0, 0, 0, 0);
    
    // 启用链接检测和交互
    m_pTextBrowser->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::LinksAccessibleByMouse);
    
    // 设置大小策略 - 宽度适应父窗口，高度适应内容
    m_pTextBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    connect(m_pTextBrowser, &QTextBrowser::anchorClicked, this, &McpServerListItemTooltip::onAnchorClicked);
    m_pLayout->addWidget(m_pTextBrowser);
    m_pLayout->setStretchFactor(m_pTextBrowser, 1);
}
