#include "chatbotwidget.h"
#include "chatbotplatformdialog.h"
#include "themedlable.h"
#include "chatbotservice.h"
#include "serverwrapper.h"

#include <DFontSizeManager>
#include <DGuiApplicationHelper>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLoggingCategory>

using namespace uos_ai;
using namespace uos_ai::chatbot;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

// ──────────────────────────────────────────────
// PlatformRowWidget
// ──────────────────────────────────────────────

PlatformRowWidget::PlatformRowWidget(const QString &name, const QString &key, DWidget *parent)
    : DWidget(parent), m_key(key)
{
    m_nameLabel = new DLabel(name, this);
    DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, QFont::Medium);

    m_editBtn = new QToolButton(this);
    m_editBtn->setIcon(QIcon::fromTheme("chatbot-settings"));
    m_editBtn->setIconSize(QSize(16, 16));
    m_editBtn->setAutoRaise(true);
    m_editBtn->setVisible(false);

    m_sw = new DSwitchButton(this);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 10, 10, 10);
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_editBtn, 0, Qt::AlignVCenter);
    layout->addStretch();
    layout->addWidget(m_sw, 0, Qt::AlignVCenter);

    connect(m_sw, &DSwitchButton::checkedChanged, this, [this](bool checked) {
        m_editBtn->setVisible(checked);
        emit platformEnableChanged(m_key, checked);
    });
    connect(m_editBtn, &QToolButton::clicked, this, [this]() {
        emit configureClicked(m_key);
    });
}

void PlatformRowWidget::setChecked(bool checked)
{
    m_sw->blockSignals(true);
    m_sw->setChecked(checked);
    m_sw->blockSignals(false);
    m_editBtn->setVisible(checked);
}

// ──────────────────────────────────────────────
// ChatBotWidget
// ──────────────────────────────────────────────

ChatBotWidget::ChatBotWidget(DWidget *parent)
    : DWidget(parent)
{
    m_service = ServerWrapper::instance()->chatBotService();
    if (m_service) {
        connect(m_service, &ChatBotService::configChanged,
                this, &ChatBotWidget::onServiceConfigChanged);
    }

    initUI();
    loadFromService();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &ChatBotWidget::onThemeTypeChanged);
    onThemeTypeChanged();
}

QString ChatBotWidget::getTitleName()
{
    return tr("IM Integration");
}

void ChatBotWidget::initUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_titleLabel = new ThemedLable(tr("Third-party IM Integration"), this);
    m_titleLabel->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T5, QFont::Bold);

    layout->addWidget(m_titleLabel, 0, Qt::AlignLeft);
    layout->addSpacing(10);
    layout->addWidget(createPlatformGroup());
    layout->addStretch();
}

DBackgroundGroup *ChatBotWidget::createPlatformGroup()
{
    // ── enable row ──────────────────────────────
    DWidget *enableRow = new DWidget(this);
    enableRow->setMinimumHeight(56);

    m_mainSwitch = new DSwitchButton(enableRow);

    DLabel *nameLabel = new DLabel(tr("Enable Message Forwarding Service"), enableRow);
    DFontSizeManager::instance()->bind(nameLabel, DFontSizeManager::T6, QFont::Medium);

    const QString descText = tr("After enabling, UOS AI will receive messages from Lark, DingTalk, and QQ through the configured bot. You can then directly interact with UOS AI in your IM client.");
    DLabel *descLabel = new DLabel(descText, enableRow);
    DFontSizeManager::instance()->bind(descLabel, DFontSizeManager::T8, QFont::Normal);
    descLabel->setElideMode(Qt::ElideRight);
    descLabel->setToolTip(QString("<p>%1</p>").arg(descText.toHtmlEscaped()));
    DPalette descPalette = descLabel->palette();
    descPalette.setColor(QPalette::WindowText,
                         DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::TextTips));
    descLabel->setPalette(descPalette);

    QVBoxLayout *textLayout = new QVBoxLayout;
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    textLayout->addWidget(nameLabel);
    textLayout->addWidget(descLabel);

    QHBoxLayout *enableLayout = new QHBoxLayout(enableRow);
    enableLayout->setContentsMargins(10, 1, 10, 1);
    enableLayout->addLayout(textLayout);
    enableLayout->addStretch();
    enableLayout->addWidget(m_mainSwitch, 0, Qt::AlignVCenter);

    connect(m_mainSwitch, &DSwitchButton::checkedChanged,
            this, &ChatBotWidget::onMainEnableChanged);

    // ── platform rows ────────────────────────────
    struct PlatformDef { QString key; QString name; };
    const QList<PlatformDef> platforms = {
        { "feishu",   tr("Lark")     },
        { "dingtalk", tr("DingTalk") },
        { "qq",       tr("QQ")       },
    };

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->addWidget(enableRow);

    for (const auto &p : platforms) {
        auto *rowWidget = new PlatformRowWidget(p.name, p.key, this);
        rowWidget->setVisible(false);
        vLayout->addWidget(rowWidget);

        connect(rowWidget, &PlatformRowWidget::platformEnableChanged,
                this, &ChatBotWidget::onPlatformEnableChanged);
        connect(rowWidget, &PlatformRowWidget::configureClicked,
                this, &ChatBotWidget::onConfigureClicked);

        m_platformRows[p.key] = { rowWidget->sw(), rowWidget };
    }

    m_mainGroup = new DBackgroundGroup(vLayout, this);
    m_mainGroup->setContentsMargins(0, 0, 0, 0);
    m_mainGroup->setItemSpacing(1);
    return m_mainGroup;
}

void ChatBotWidget::updatePlatformGroupVisible(bool enabled)
{
    for (auto it = m_platformRows.begin(); it != m_platformRows.end(); ++it)
        it.value().row->setVisible(enabled);
}

// ──────────────────────────────────────────────
// Config loading / saving
// ──────────────────────────────────────────────

void ChatBotWidget::loadFromService()
{
    if (!m_service)
        return;

    m_config = m_service->config();
    if (m_config.isEmpty())
        return;

    m_mainSwitch->blockSignals(true);
    m_mainSwitch->setChecked(m_config.value("enabled").toBool(false));
    m_mainSwitch->blockSignals(false);

    updatePlatformGroupVisible(m_config.value("enabled").toBool(false));

    QJsonObject platforms = m_config.value("platforms").toObject();
    for (auto it = m_platformRows.begin(); it != m_platformRows.end(); ++it) {
        QJsonObject pCfg = platforms.value(it.key()).toObject();
        it.value().row->setChecked(pCfg.value("enabled").toBool(false));
    }
}

void ChatBotWidget::saveToService()
{
    if (!m_service)
        return;
    m_service->applyConfig(m_config);
}

// ──────────────────────────────────────────────
// Slots
// ──────────────────────────────────────────────

void ChatBotWidget::onThemeTypeChanged()
{
    if (!m_mainGroup) return;
    DPalette pl = m_mainGroup->palette();
    pl.setBrush(DPalette::Base,
                DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_mainGroup->setPalette(pl);
}

void ChatBotWidget::onMainEnableChanged(bool enabled)
{
    m_config["enabled"] = enabled;
    updatePlatformGroupVisible(enabled);
    saveToService();
}

void ChatBotWidget::onPlatformEnableChanged(const QString &key, bool enabled)
{
    QJsonObject platforms = m_config.value("platforms").toObject();
    QJsonObject pCfg      = platforms.value(key).toObject();
    pCfg["enabled"]       = enabled;
    platforms[key]        = pCfg;
    m_config["platforms"] = platforms;
    saveToService();
}

void ChatBotWidget::onConfigureClicked(const QString &key)
{
    ChatBotPlatformDialog dlg(key, this);

    QJsonObject platforms = m_config.value("platforms").toObject();
    dlg.setConfig(platforms.value(key).toObject());

    if (dlg.exec() != QDialog::Accepted)
        return;

    QJsonObject newFields = dlg.config();
    QJsonObject existing  = platforms.value(key).toObject();
    for (auto it = newFields.begin(); it != newFields.end(); ++it)
        existing[it.key()] = it.value();

    platforms[key]        = existing;
    m_config["platforms"] = platforms;
    saveToService();

    qCInfo(logAIGUI) << "Chatbot platform credentials updated:" << key;
}

void ChatBotWidget::onServiceConfigChanged(const QJsonObject &config)
{
    if (config == m_config)
        return;

    m_config = config;

    m_mainSwitch->blockSignals(true);
    m_mainSwitch->setChecked(config.value("enabled").toBool(false));
    m_mainSwitch->blockSignals(false);
    updatePlatformGroupVisible(config.value("enabled").toBool(false));

    QJsonObject platforms = config.value("platforms").toObject();
    for (auto it = m_platformRows.begin(); it != m_platformRows.end(); ++it) {
        QJsonObject pCfg = platforms.value(it.key()).toObject();
        it.value().row->setChecked(pCfg.value("enabled").toBool(false));
    }
}
