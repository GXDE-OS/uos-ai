#include "chatbotplatformdialog.h"
#include "chatbot_key_define.h"
#include "global_key_define.h"

#include <DTitlebar>
#include <DLabel>
#include <DFontSizeManager>
#include <DPaletteHelper>
#include <DGuiApplicationHelper>

#include <QDesktopServices>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLoggingCategory>
#include <QUrl>
#include <QVBoxLayout>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

ChatBotPlatformDialog::ChatBotPlatformDialog(const QString &platformKey, DWidget *parent)
    : DAbstractDialog(parent)
    , m_platformKey(platformKey)
{
    initUI();
    initConnect();
    setModal(true);
}

void ChatBotPlatformDialog::initUI()
{
    setFixedWidth(440);

    // 标题栏
    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);

    QString title;
    if (m_platformKey == QLatin1String(STR_PLATFORM_FEISHU))
        title = tr("Lark Integration Settings");
    else if (m_platformKey == QLatin1String(STR_PLATFORM_DINGTALK))
        title = tr("DingTalk Integration Settings");
    else if (m_platformKey == QLatin1String(STR_PLATFORM_TELEGRAM))
        title = tr("Telegram Integration Settings");
    else if (m_platformKey == QLatin1String(STR_PLATFORM_DISCORD))
        title = tr("Discord Integration Settings");
    else
        title = tr("QQ Integration Settings");
    titleBar->setTitle(title);
    DFontSizeManager::instance()->bind(titleBar, DFontSizeManager::T5, QFont::DemiBold);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setSpacing(10);

    for (const FieldConfig &config : fieldConfigs()) {
        FieldWidgets field;
        field.config = config;
        field.label = new DLabel(config.label, this);
        if (config.password) {
            field.edit = new DPasswordEdit(this);
        } else {
            auto *lineEdit = new DLineEdit(this);
            lineEdit->setClearButtonEnabled(true);
            field.edit = lineEdit;
        }
        field.edit->setPlaceholderText(config.required ? tr("Required") : tr("Optional"));
        formLayout->addRow(field.label, field.edit);
        m_fields.append(field);
    }

    // 配置方法链接（居中显示）
    m_helpLabel = new DLabel(this);
    m_helpLabel->setAlignment(Qt::AlignCenter);
    m_helpLabel->setOpenExternalLinks(false);

    QHBoxLayout *helpLayout = new QHBoxLayout;
    helpLayout->setContentsMargins(0, 0, 0, 0);
    helpLayout->addStretch();
    helpLayout->addWidget(m_helpLabel);
    helpLayout->addStretch();

    // 按钮区
    m_cancelBtn  = new DPushButton(tr("Cancel"), this);
    m_confirmBtn = new DSuggestButton(tr("Confirm"), this);
    m_confirmBtn->setEnabled(false);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(10);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_confirmBtn);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 0, 20, 20);
    mainLayout->setSpacing(16);
    mainLayout->addWidget(titleBar);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(helpLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}

void ChatBotPlatformDialog::initConnect()
{
    for (const FieldWidgets &field : std::as_const(m_fields))
        connect(field.edit, &DLineEdit::textChanged, this, &ChatBotPlatformDialog::updateConfirmEnabled);

    connect(m_cancelBtn,  &DPushButton::clicked, this, &ChatBotPlatformDialog::reject);
    connect(m_confirmBtn, &DSuggestButton::clicked, this, &ChatBotPlatformDialog::accept);

    connect(m_helpLabel, &DLabel::linkActivated, this, [](const QString &link) {
        QDesktopServices::openUrl(QUrl(link));
    });

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &ChatBotPlatformDialog::updateHelpLabel);
    updateHelpLabel();
}

void ChatBotPlatformDialog::updateConfirmEnabled()
{
    bool enabled = true;
    for (const FieldWidgets &field : std::as_const(m_fields)) {
        if (field.config.required && field.edit->text().trimmed().isEmpty()) {
            enabled = false;
            break;
        }
    }
    m_confirmBtn->setEnabled(enabled);
}

void ChatBotPlatformDialog::setConfig(const QJsonObject &cfg)
{
    for (const FieldWidgets &field : std::as_const(m_fields))
        field.edit->setText(cfg.value(field.config.key).toString());
    updateConfirmEnabled();
}

QJsonObject ChatBotPlatformDialog::config() const
{
    QJsonObject obj;
    for (const FieldWidgets &field : m_fields)
        obj[field.config.key] = field.edit->text().trimmed();
    return obj;
}

QVector<ChatBotPlatformDialog::FieldConfig> ChatBotPlatformDialog::fieldConfigs() const
{
    if (m_platformKey == QLatin1String(STR_PLATFORM_FEISHU)) {
        return {
            { QStringLiteral("app_id"), QStringLiteral("App ID"), true, true },
            { QStringLiteral("app_secret"), QStringLiteral("App Secret"), true, true },
        };
    }

    if (m_platformKey == QLatin1String(STR_PLATFORM_DINGTALK)) {
        return {
            { QStringLiteral("client_id"), QStringLiteral("Client ID"), true, true },
            { QStringLiteral("client_secret"), QStringLiteral("Client Secret"), true, true },
            { QStringLiteral("card_template_id"), QStringLiteral("Card Template ID"), false, false },
        };
    }

    if (m_platformKey == QLatin1String(STR_PLATFORM_TELEGRAM)) {
        return {
            { QStringLiteral("bot_token"), QStringLiteral("Bot Token"), true, true },
            { QStringLiteral("api_base"), QStringLiteral("API Base"), false, false },
        };
    }

    if (m_platformKey == QLatin1String(STR_PLATFORM_DISCORD)) {
        return {
            { QStringLiteral("bot_token"), QStringLiteral("Bot Token"), true, true },
            { QStringLiteral("application_id"), QStringLiteral("Application ID"), true, true },
            { QStringLiteral("guild_id"), QStringLiteral("Guild ID"), false, false },
        };
    }

    return {
        { QStringLiteral("app_id"), QStringLiteral("AppID"), true, true },
        { QStringLiteral("token"), QStringLiteral("AppSecret"), true, true },
    };
}

void ChatBotPlatformDialog::updateHelpLabel()
{
    const QColor color = DPaletteHelper::instance()->palette(m_helpLabel).color(DPalette::Normal, DPalette::Highlight);

    QString url;
    if (m_platformKey == QLatin1String(STR_PLATFORM_FEISHU))
        url = QStringLiteral("https://bbs.deepin.org/post/296336");
    else if (m_platformKey == QLatin1String(STR_PLATFORM_DINGTALK))
        url = QStringLiteral("https://bbs.deepin.org/post/296337");
    else if (m_platformKey == QLatin1String(STR_PLATFORM_TELEGRAM))
        url = QStringLiteral("https://core.telegram.org/bots/api");
    else if (m_platformKey == QLatin1String(STR_PLATFORM_DISCORD))
        url = QStringLiteral("https://discord.com/developers/docs/intro");
    else
        url = QStringLiteral("https://bbs.deepin.org/post/296334");

    m_helpLabel->setText(
        QString("<a href=\"%1\" style=\"color:%2; text-decoration: none;\">%3</a>")
            .arg(url, color.name(), tr("Configuration Guide >")));
}
