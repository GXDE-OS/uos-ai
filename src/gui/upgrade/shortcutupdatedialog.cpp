#include "shortcutupdatedialog.h"
#include "dbwrapper.h"
#include "private/echatwndmanager.h"
#include "utils/util.h"
#include "private/themedlable.h"
#include "private/eaiexecutor.h"
#include "shortcutmanager.h"

#include <DDialog>
#include <DFontSizeManager>
#include <DPaletteHelper>
#include <DTitlebar>

#include <QThread>
#include <QDebug>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QLoggingCategory>
#include <QApplication>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

static int m_agentValue = 0;

ShortcutUpdateDialog::ShortcutUpdateDialog(QWidget *parent)
    : DAbstractDialog(parent)
{
    m_currentPromptCount = DbWrapper::localDbWrapper().getUpdatePromptBits(UpdatePromptBitType::SHORTCUT_UPDATE);
    setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
    initUI();
    initConnect();
    setModal(true);
}

void ShortcutUpdateDialog::initUI()
{
    setFixedWidth(381);

    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setAccessibleName("ShortcutUpdateDialogTitleBar");
    QIcon titleIcon(":assets/images/tips.svg");
    titleBar->setIcon(titleIcon);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);

    m_textLabel = new DLabel(tr("Shortcut: [Super + Space]. Quickly invoke UOS AI to access all AI features in one place."), this);
    m_textLabel->setWordWrap(true);
    m_textLabel->setAlignment(Qt::AlignCenter);
    m_textLabel->setFixedWidth(300);
    m_textLabel->setContentsMargins(15, 0, 15, 0);
    DFontSizeManager::instance()->bind(m_textLabel, DFontSizeManager::T6, QFont::Medium);

    auto labelLayout = new QHBoxLayout();
    labelLayout->setContentsMargins(0, 0, 0, 0);
    labelLayout->addStretch();
    labelLayout->addWidget(m_textLabel, 0, Qt::AlignCenter);
    labelLayout->addStretch();

    m_imageLabel = new DLabel(this);
    changeDisplayImage();
    m_imageLabel->setContentsMargins(0, 0, 0, 0);

    m_tipLabel = new ThemedLable(this);
    m_tipLabel->setPaletteColor(QPalette::Text, QPalette::BrightText, 0.5);
    m_tipLabel->setTextFormat(Qt::RichText);
    m_tipLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_tipLabel->setOpenExternalLinks(false);
    m_tipLabel->setWordWrap(true);
    m_tipLabel->setAlignment(Qt::AlignCenter);
    m_tipLabel->setFixedWidth(300);
    m_tipLabel->setContentsMargins(0, 0, 0, 0);
    DFontSizeManager::instance()->bind(m_tipLabel, DFontSizeManager::T8, QFont::Normal);

    m_confirmBtn = new DSuggestButton(tr("Got it"), this);
    m_confirmBtn->setFixedWidth(360);
    m_confirmBtn->setContentsMargins(8, 0, 8, 0);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 8);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(titleBar);
    mainLayout->addLayout(labelLayout);
    mainLayout->addSpacing(12);
    mainLayout->addWidget(m_imageLabel, 0, Qt::AlignCenter);
    mainLayout->addSpacing(12);
    mainLayout->addWidget(m_tipLabel, 0, Qt::AlignCenter);
    mainLayout->addSpacing(8);
    mainLayout->addWidget(m_confirmBtn, 0, Qt::AlignCenter);

    resetLinkColor();
    setLayout(mainLayout);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    adjustSize();
}

void ShortcutUpdateDialog::resetLinkColor()
{
    int shortcutValue = DbWrapper::localDbWrapper().getUpdatePromptBits(UpdatePromptBitType::SHORTCUT_UPDATE);
    int remainingTimes = DbWrapper::maxShowUpdatePromptTimes() - shortcutValue - 1;
    QColor linkColor = DPaletteHelper::instance()->palette(m_tipLabel).color(DPalette::Normal, DPalette::Highlight);
    QString tipText = tr("This prompt will appear %1 more times, <a href=\"#nomore\" style=\"color: %2; text-decoration: none;\">Do not show again</a>")
                     .arg(remainingTimes)
                     .arg(linkColor.name());
    m_tipLabel->setText(tipText);
}

void ShortcutUpdateDialog::onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &)
{
    resetLinkColor();
    changeDisplayImage();
}

void ShortcutUpdateDialog::initConnect()
{
    connect(m_confirmBtn, &DSuggestButton::clicked, this, [this]() {
        DbWrapper::localDbWrapper().updateUpdatePromptBits(UpdatePromptBitType::SHORTCUT_UPDATE, m_currentPromptCount + 1);
        close();
    });

    connect(m_tipLabel, &DLabel::linkActivated, this, [this](const QString &link) {
        if (link == "#nomore") {
            DbWrapper::localDbWrapper().updateUpdatePromptBits(UpdatePromptBitType::SHORTCUT_UPDATE, DbWrapper::maxShowUpdatePromptTimes());
            close();
        }
    });

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ShortcutUpdateDialog::onUpdateSystemTheme);
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));
}

void ShortcutUpdateDialog::onUpdateSystemFont(const QFont &)
{
    this->adjustSize();
}

bool ShortcutUpdateDialog::checkAndShow(QWidget *parent)
{
    if (QThread::currentThread() != qApp->thread()) {
        Q_ASSERT_X(QThread::currentThread() == qApp->thread(), __func__, "invalid working thread");
        qCCritical(logAIGUI) << " upgrade processing is not called in gui thread.";
        return false;
    }

    // v2.13 新版写作助手
    {
        int value = DbWrapper::localDbWrapper().getUpdatePromptBits(UpdatePromptBitType::NEW_WRITING);
        qCInfo(logAIGUI) << "NEW_WRITING db value:" << value;
        if (value == 0) {
            EAiExec()->showPromptWindow();
        }
        return true;
    }

    // v2.12 MCP自动模式
    {
        int value = DbWrapper::localDbWrapper().getUpdatePromptBits(UpdatePromptBitType::AUTO_MCP);
        int isShowFreeCreditsGuide = DbWrapper::localDbWrapper().getUpdatePromptBits(UpdatePromptBitType::FREE_CREDITS);
        qCInfo(logAIGUI) << "PRIVACY_UPDATE db value:" << value  << isShowFreeCreditsGuide;

        bool isPreShow = false;
        if (value == 0) {
            EAiExec()->showPromptWindow();
        } else {
            isPreShow = true;
        }

        if (isShowFreeCreditsGuide == 0 && EAiExec()->getIsShowFreeAccountGuide()) {
            EAiExec() ->changeFreeAccountGuide(true, isPreShow);
        } else {
            EAiExec() ->changeFreeAccountGuide(false, isPreShow);
        }
        return true;
    }

    // v2.10 隐私对话新手引导
    {
        int value = DbWrapper::localDbWrapper().getUpdatePromptBits(UpdatePromptBitType::PRIVACY_UPDATE);
        qCInfo(logAIGUI) << "PRIVACY_UPDATE db value:" << value;
        if (value == 0) {
            EAiExec()->showPromptWindow();
        }
        return true;
    }

    // v2.9 MCP新手引导
    {
        int value = DbWrapper::localDbWrapper().getUpdatePromptBits(UpdatePromptBitType::MCP_UPDATE);
        qCInfo(logAIGUI) << "MCP_UPDATE db value:" << value;
        if (value == 0) {
            EAiExec()->showPromptWindow();
        }
        return true;
    }

    // v2.8 快捷键+智能体新手引导
    {
        //快捷键更新提示窗出现条件：
        //1.快捷键是否更新成功
        //2.数据库查询
        int shortcutValue = needUpdatePrompt();
        // 使用枚举获取智能体控制位
        int agentValue = DbWrapper::localDbWrapper().getUpdatePromptBits(UpdatePromptBitType::AGENT_UPDATE);
        // 快捷键新手引导，后端实现
        if (shortcutValue < DbWrapper::maxShowUpdatePromptTimes() && isShortcutRight()) {
            ShortcutUpdateDialog dialog(parent);
            EWndManager()->registeWindow(&dialog); // m_mask登记
            dialog.adjustSize(); // 确保对话框根据内容调整大小
            dialog.setDisplayPosition(DisplayPosition::Center);
            dialog.exec();
            EWndManager()->unregisteWindow(&dialog);
        }

        // 前端智能体新手引导，前端实现
        if (agentValue == 0) {
            EAiExec()->showPromptWindow();
        }
    }
    return true;
}

bool ShortcutUpdateDialog::isShortcutRight()
{
    ShortcutManager& shortcutMgr = ShortcutManager::getInstance();

    if (!shortcutMgr.isValid()) {
        qCWarning(logAIGUI) << "ShortcutManager D-Bus interface is not valid";
        return false;
    }

    QList<ShortcutInfo> shortcuts = shortcutMgr.searchShortcuts("UOS AI");

    for (const ShortcutInfo &shortcut : shortcuts) {
        if (shortcut.id == "UOS AI" && shortcut.accel == "<Super>space") {
            return true;
        }
    }
    return false;
}

int ShortcutUpdateDialog::needUpdatePrompt()
{
    return DbWrapper::localDbWrapper().getUpdatePromptBits(UpdatePromptBitType::SHORTCUT_UPDATE);
}

void ShortcutUpdateDialog::changeDisplayImage()
{
    QString iconName = QString(":/icons/deepin/builtin/%1/icons/");
    if (Util::checkLanguage())
        iconName += "shortcut.svg";
    else
        iconName += "shortcut_en.svg";

    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
        iconName = iconName.arg("light");
    else
        iconName = iconName.arg("dark");
    m_imageLabel->setPixmap(Util::loadSvgPixmap(iconName, QSize(198, 104)));
}
