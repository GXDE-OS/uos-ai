#include "chatbotplatformdialog.h"

#include <DTitlebar>
#include <DLabel>
#include <DFontSizeManager>
#include <DPaletteHelper>
#include <DGuiApplicationHelper>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLoggingCategory>
#include <QDesktopServices>
#include <QUrl>

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
    if (m_platformKey == "feishu")
        title = tr("Lark Integration Settings");
    else if (m_platformKey == "dingtalk")
        title = tr("DingTalk Integration Settings");
    else
        title = tr("QQ Integration Settings");
    titleBar->setTitle(title);
    DFontSizeManager::instance()->bind(titleBar, DFontSizeManager::T5, QFont::DemiBold);

    QString label1, label2, label3;
    if (m_platformKey == "feishu") {
        label1 = "App ID";
        label2 = "App Secret";
    } else if (m_platformKey == "dingtalk") {
        label1 = "Client ID";
        label2 = "Client Secret";
        label3 = "Card Template ID";
    } else { // qq
        label1 = "AppID";
        label2 = "AppSecret";
    }

    // 表单
    m_field1Edit = new DPasswordEdit(this);
    m_field1Edit->setPlaceholderText(tr("Required"));

    m_field2Edit = new DPasswordEdit(this);
    m_field2Edit->setPlaceholderText(tr("Required"));

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setSpacing(10);
    formLayout->addRow(new DLabel(label1), m_field1Edit);
    formLayout->addRow(new DLabel(label2), m_field2Edit);

    if (m_platformKey == "dingtalk") {
        m_field3Edit = new DLineEdit(this);
        m_field3Edit->setPlaceholderText(tr("Optional"));
        m_field3Edit->setClearButtonEnabled(true);
        formLayout->addRow(new DLabel(label3), m_field3Edit);
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
    connect(m_field1Edit, &DPasswordEdit::textChanged, this, &ChatBotPlatformDialog::updateConfirmEnabled);
    connect(m_field2Edit, &DPasswordEdit::textChanged, this, &ChatBotPlatformDialog::updateConfirmEnabled);
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
    m_confirmBtn->setEnabled(
        !m_field1Edit->text().trimmed().isEmpty() &&
        !m_field2Edit->text().trimmed().isEmpty()
    );
}

void ChatBotPlatformDialog::setConfig(const QJsonObject &cfg)
{
    if (m_platformKey == "feishu") {
        m_field1Edit->setText(cfg.value("app_id").toString());
        m_field2Edit->setText(cfg.value("app_secret").toString());
    } else if (m_platformKey == "dingtalk") {
        m_field1Edit->setText(cfg.value("client_id").toString());
        m_field2Edit->setText(cfg.value("client_secret").toString());
        m_field3Edit->setText(cfg.value("card_template_id").toString());
    } else { // qq
        m_field1Edit->setText(cfg.value("app_id").toString());
        m_field2Edit->setText(cfg.value("token").toString());
    }
    updateConfirmEnabled();
}

QJsonObject ChatBotPlatformDialog::config() const
{
    QJsonObject obj;
    if (m_platformKey == "feishu") {
        obj["app_id"]     = m_field1Edit->text().trimmed();
        obj["app_secret"] = m_field2Edit->text().trimmed();
    } else if (m_platformKey == "dingtalk") {
        obj["client_id"]        = m_field1Edit->text().trimmed();
        obj["client_secret"]    = m_field2Edit->text().trimmed();
        obj["card_template_id"] = m_field3Edit->text().trimmed();
    } else { // qq
        obj["app_id"] = m_field1Edit->text().trimmed();
        obj["token"]  = m_field2Edit->text().trimmed();
    }
    return obj;
}

void ChatBotPlatformDialog::updateHelpLabel()
{
    const QColor color = DPaletteHelper::instance()->palette(m_helpLabel).color(DPalette::Normal, DPalette::Highlight);

    QString url;
    if (m_platformKey == "feishu")
        url = "https://bbs.deepin.org/post/296336";
    else if (m_platformKey == "dingtalk")
        url = "https://bbs.deepin.org/post/296337";
    else
        url = "https://bbs.deepin.org/post/296334";

    m_helpLabel->setText(
        QString("<a href=\"%1\" style=\"color:%2; text-decoration: none;\">%3</a>")
        .arg(url, color.name(), tr("Configuration Guide >"))
    );
}
