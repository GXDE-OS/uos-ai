#include "custommcpservereditor.h"
#include "uosai_global.h"
#include "utils/util.h"
#include "agentfactory.h"
#include "mcpserver.h"
#include "linenumbertextedit.h"
#include "global_define.h"

#include <DTitlebar>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageBox>
#include <QLoggingCategory>
#include <QFrame>
#include <QSpacerItem>
#include <QToolTip>
#include <QHelpEvent>
#include <QFontMetrics>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

CustomMcpServerEditor::CustomMcpServerEditor(DWidget *parent)
    : DAbstractDialog(parent)
{
    initUI();
    initConnect();
    setModal(true);
}

bool CustomMcpServerEditor::showAddMode()
{
    m_currentMode = Add;
    setTitle(tr("Add MCP Server"));
    m_jsonConfigEdit->setPlainText(defualtJsonConfig());

    return exec();
}

bool CustomMcpServerEditor::showEditMode(const QJsonObject &serverConfig, const QString &description)
{
    m_currentMode = Edit;
    setTitle(tr("Edit MCP Server"));

    // 检查传入的配置是否已经包含在mcpServers字段下
    QJsonObject configToDisplay;
    if (serverConfig.contains("mcpServers") && serverConfig["mcpServers"].isObject()) {
        // 如果已经包含mcpServers字段，直接使用原配置
        configToDisplay = serverConfig;
    } else {
        configToDisplay["mcpServers"] = serverConfig;
    }

    QJsonDocument doc(configToDisplay);
    m_jsonConfigEdit->setPlainText(doc.toJson(QJsonDocument::Indented));

    if (!description.isEmpty())
        m_descriptionEdit->setPlainText(description);

    return exec();
}

void CustomMcpServerEditor::initUI()
{
    // 设置对话框大小
    setFixedWidth(600);

    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);
    titleBar->setFixedWidth(600);
    titleBar->setIcon(QIcon(":/assets/images/uos-ai-assistant.svg"));

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 36, 10, 10);
    mainLayout->setSpacing(15);
    
    // 标题
    m_titleLabel = new DLabel(tr("Add MCP Server"), this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    auto titleLabelPa = m_titleLabel->palette();
    titleLabelPa.setColor(QPalette::WindowText, titleLabelPa.color(QPalette::BrightText));
    m_titleLabel->setPalette(titleLabelPa);
    DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T5, QFont::DemiBold);
    mainLayout->addWidget(m_titleLabel, Qt::AlignHCenter);
    
    // JSON配置编辑框
    QHBoxLayout *jsonLabelLayout = new QHBoxLayout;
    jsonLabelLayout->setContentsMargins(10, 0, 0, 0);
    jsonLabelLayout->setSpacing(3);
    DLabel *jsonLabel = new DLabel(tr("JSON configuration:"), this);
    DFontSizeManager::instance()->bind(jsonLabel, DFontSizeManager::T6, QFont::Medium);
    jsonLabelLayout->addWidget(jsonLabel);

    DLabel *jsonTipLabel = new DLabel(tr("Please paste the MCP JSON configuration code into the input box."), this);
    DFontSizeManager::instance()->bind(jsonTipLabel, DFontSizeManager::T6, QFont::Normal);
    // 设置尾部缺省显示
    jsonTipLabel->setWordWrap(false);
    jsonTipLabel->setElideMode(Qt::ElideRight);
    // 设置tooltip为完整文本，但仅在缺省显示时显示
    jsonTipLabel->setToolTip(tr("Please paste the MCP JSON configuration code into the input box."));
    // 重写paintEvent来动态控制tooltip显示
    jsonTipLabel->installEventFilter(this);

    jsonLabelLayout->addWidget(jsonTipLabel);
    jsonLabelLayout->addStretch(1);
    mainLayout->addLayout(jsonLabelLayout);
    
    m_jsonConfigEdit = new LineNumberTextEdit(this);
    m_jsonConfigEdit->setFixedHeight(145);
    DFontSizeManager::instance()->bind(m_jsonConfigEdit, DFontSizeManager::T6, QFont::Normal);
    mainLayout->addWidget(m_jsonConfigEdit);
    
    // 描述编辑框
    DLabel *descLabel = new DLabel(tr("Describe"), this);
    descLabel->setContentsMargins(10, 0, 0, 0);
    DFontSizeManager::instance()->bind(descLabel, DFontSizeManager::T6, QFont::Medium);
    mainLayout->addWidget(descLabel);
    
    m_descriptionEdit = new DTextEdit(this);
    m_descriptionEdit->setPlaceholderText(tr("Describe MCP server functions to facilitate quick search tools"));
    m_descriptionEdit->setFixedHeight(100);
    DFontSizeManager::instance()->bind(m_descriptionEdit, DFontSizeManager::T6, QFont::Normal);
    mainLayout->addWidget(m_descriptionEdit);

    // 错误标签（初始隐藏）
    m_errorLabel = new DLabel(this);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setAlignment(Qt::AlignHCenter);
    m_errorLabel->setContentsMargins(10, 0, 10, 0);
    DFontSizeManager::instance()->bind(m_errorLabel, DFontSizeManager::T7, QFont::Normal);
    
    // 设置错误标签样式为红色
    auto errorPalette = m_errorLabel->palette();
    errorPalette.setColor(QPalette::WindowText, QColor(255, 87, 54));
    m_errorLabel->setPalette(errorPalette);
    m_errorLabel->hide();  // 初始隐藏
    
    mainLayout->addWidget(m_errorLabel);
    
    // 添加弹性空间
    mainLayout->addStretch();
    
    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(10);

    // 取消按钮
    m_cancelButton = new DPushButton(tr("Cancel"), this);
    m_cancelButton->setFixedHeight(36);
    buttonLayout->addWidget(m_cancelButton);
    
    // 分隔线
    QFrame *separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setFixedHeight(30);
    buttonLayout->addWidget(separator);
    
    // 确定按钮
    m_okButton = new DSuggestButton(tr("Confirm"), this);
    m_okButton->setFixedHeight(36);
    m_okButton->setEnabled(false); // 初始禁用
    buttonLayout->addWidget(m_okButton);
    
    mainLayout->addLayout(buttonLayout);
}

void CustomMcpServerEditor::initConnect()
{
    connect(m_okButton, &DSuggestButton::clicked, this, &CustomMcpServerEditor::onOkButtonClicked);
    connect(m_cancelButton, &DPushButton::clicked, this, &CustomMcpServerEditor::onCancelButtonClicked);
    connect(m_jsonConfigEdit, &LineNumberTextEdit::textChanged, this, &CustomMcpServerEditor::updateOkButtonStatus);
}

QJsonObject CustomMcpServerEditor::getServerConfig() const
{
    QString jsonText = m_jsonConfigEdit->toPlainText().trimmed();
    if (jsonText.isEmpty())
        return QJsonObject();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError && doc.isObject())
        return assembleDescritions(doc.object());

    return QJsonObject();
}

void CustomMcpServerEditor::setTitle(const QString &title)
{
    if (m_titleLabel) 
        m_titleLabel->setText(title);
}

// 添加事件过滤器来处理tooltip显示逻辑
bool CustomMcpServerEditor::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);
        DLabel *label = qobject_cast<DLabel*>(obj);
        
        if (label && label->toolTip().isEmpty() == false) {
            // 检查文本是否被截断（缺省显示）
            QFontMetrics fm(label->font());
            bool isElided = fm.horizontalAdvance(label->text()) > label->width();
            
            // 仅在文本被截断时显示tooltip
            if (isElided) {
                QToolTip::showText(helpEvent->globalPos(), label->toolTip(), label);
            } else {
                QToolTip::hideText();
            }
            return true;
        }
    }
    return DAbstractDialog::eventFilter(obj, event);
}

void CustomMcpServerEditor::onOkButtonClicked()
{
    QSharedPointer<MCPServer> mcpServer = AgentFactory::instance()->getMCPServer(kDefaultAgentName);
    if (!mcpServer) {
        qCWarning(logAIGUI) << QString("Can't find mcp server for agent: %0").arg(kDefaultAgentName);
        return;
    }

    mcpServer->scanServers();
    auto serverConfig = getServerConfig();
    QJsonDocument doc(serverConfig);

    QPair<bool, QString> res;
    if (m_currentMode == Add) {
        res = mcpServer->importCustomServers(doc.toJson());
    } else {
        res = mcpServer->updateCustomServers(doc.toJson());
    }

    if (res.first) {
        // 成功时隐藏错误标签
        showErrorMessage("");
        accept();
    } else {
        // 失败时显示错误信息
        showErrorMessage(res.second);
    }
}

void CustomMcpServerEditor::onCancelButtonClicked()
{
    reject();
}

void CustomMcpServerEditor::updateOkButtonStatus()
{
    bool valid = !m_jsonConfigEdit->toPlainText().trimmed().isEmpty() &&
                 validateJsonConfig();
    
    m_okButton->setEnabled(valid);
}

bool CustomMcpServerEditor::validateJsonConfig() const
{
    QString jsonText = m_jsonConfigEdit->toPlainText().trimmed();
    if (jsonText.isEmpty())
        return false;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    // 设置默认的JSON配置结构
    QJsonObject defaultConfig;
    defaultConfig["mcpServers"] = QJsonObject();

    QJsonDocument defaultDoc(defaultConfig);
    return QString(defaultDoc.toJson()).compare(QString(doc.toJson()));
}

QJsonObject CustomMcpServerEditor::assembleDescritions(const QJsonObject &config) const
{
    QString descText = m_descriptionEdit->toPlainText().trimmed();
    // 如果描述文本不为空，则更新所有MCP服务的description字段
    if (descText.isEmpty())
        return config;
    // 检查是否存在mcpServers字段
    if (!config.contains("mcpServers") || !config["mcpServers"].isObject())
        return config;

    QJsonObject mcpServers = config["mcpServers"].toObject();
    QJsonObject updatedServersConfig = config;
    QJsonObject updatedServers;

    // 遍历所有MCP服务，更新description字段
    for (auto it = mcpServers.begin(); it != mcpServers.end(); ++it) {
        QString serverName = it.key();
        QJsonValue serverValue = it.value();

        if (serverValue.isObject()) {
            QJsonObject serverConfig = serverValue.toObject();
            // 设置descriptions字段为多语言哈希结构
            QJsonObject descriptions;
            if (serverConfig.contains("descriptions") && serverConfig["descriptions"].isObject()) {
                descriptions = serverConfig["descriptions"].toObject();
            }
            const QString locale = QLocale::system().name().simplified();
            const QString shortLocale = Util::splitLocaleName(locale);

            // 将当前描述文本添加到当前系统语言的映射中
            if (!descriptions.contains("generic"))
                descriptions["generic"] = descText;
            if (!descriptions.contains(shortLocale))
                descriptions[shortLocale] = descText;
            descriptions[locale] = descText;

            serverConfig["descriptions"] = descriptions;
            updatedServers[serverName] = serverConfig;
        } else {
            // 如果不是对象，保持原样
            updatedServers[serverName] = serverValue;
        }
    }
    // 更新配置中的mcpServers
    updatedServersConfig["mcpServers"] = updatedServers;
    return updatedServersConfig;
}

QString CustomMcpServerEditor::defualtJsonConfig() const
{
    QJsonObject defaultConfig;
    defaultConfig["mcpServers"] = QJsonObject();

    QJsonDocument doc(defaultConfig);
    return doc.toJson(QJsonDocument::Indented);
}

void CustomMcpServerEditor::showErrorMessage(const QString &errMsg)
{
    if (errMsg.isEmpty()) {
        m_errorLabel->hide();
    } else {
        m_errorLabel->setText(errMsg);
        m_errorLabel->show();
    }
}
