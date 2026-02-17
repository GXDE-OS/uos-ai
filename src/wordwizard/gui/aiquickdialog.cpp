#include "aiquickdialog.h"
#include <gui/chat/private/eappaiprompt.h>
#include <llm/common/networkdefs.h>
#include <wrapper/serverwrapper.h>
#include <dbus/fcitxinputserver.h>
#include <wrapper/wizardwrapper.h>
#include <wordwizard.h>
#include <utils/dconfigmanager.h>
#include "../private/writertextedit.h"
#include "../private/continuebutton.h"
#include "../private/topdialog.h"
#include "../private/languageComboDelegate.h"
#include "util.h"
#include "utils/esystemcontext.h"
#include "private/baseclipboard.h"

#include <DWidgetUtil>
#include <DTitlebar>
#include <DLabel>
#include <DFontSizeManager>
#include <DPushButton>
#include <DPlainTextEdit>
#include <DDialog>
#include <DPalette>
#include <DPaletteHelper>
#include <DComboBox>

#include <QDesktopServices>
#include <QDebug>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QClipboard>
#include <QThread>
#include <QScreen>
#include <QToolTip>
#include <QTextBlockFormat>
#include <QActionGroup>
#include <QScrollBar>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logWordWizard)

#ifdef COMPILE_ON_V25
#include <ddeshellwayland.h>
#endif

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
DCORE_USE_NAMESPACE
UOSAI_USE_NAMESPACE

Q_DECLARE_METATYPE(CustomFunction)

static constexpr char WARNING_ICON[] = ":/assets/images/warning.svg";

QList<QString> AiQuickDialog::kLanguageList = {
    "中文（简体）",
    "中文（繁體）",
    "བོད་སྐད་དང་།",
    "English",
    "日本語",
    "Deutsch",
    "Español",
    "Français",
    "Italiano",
    "한국인",
    "Bahasa Melayu",
    "português",
    "Русский",
    "ภาษาไทย",
    "Tiếng Việt",
};

int AiQuickDialog::kTargetLanguageIdx = -1;

bool AiQuickDialog::kIsFirstTranslate = true;

LanguageComboDelegate *m_languageComboBoxDelegate = nullptr;

bool AiQuickDialog::almostAllEnglish(QString text) {
    if (text.isEmpty()) {
        return true;
    }

    int engCounts = 0;
    for (const QChar &qchar : text) {
        if (qchar.script() == QChar::Script_Latin || qchar.script() == QChar::Script_Common) {
            engCounts++;
        }
    }
    if (engCounts / static_cast<float>(text.size()) > 0.8F) {
        return true;
    }

    return false;
}

AiQuickDialog::AiQuickDialog(QObject *parent)
    : DAbstractDialog(), m_parent(parent)
{
    // 初始化自定义功能列表
    m_customFunctionList = WordWizard::kCustomFunctionList;
    // 翻译提示词列表
    m_languagePromptList = {
        tr("simplified Chinese"), // 简体中文
        tr("traditional Chinese"), // 繁体中文
        tr("Tibetan"), // 藏语
        tr("English"), // 英语
        tr("Japanese "), // 日语
        tr("German"), // 德语
        tr("Spanish"), // 西班牙语
        tr("French"), // 法语
        tr("Italian"), // 意大利语
        tr("Korean"), // 韩语
        tr("Malay"), // 马来语
        tr("Portuguese"), // 葡萄牙语
        tr("Russian"), // 俄语
        tr("Thai"), // 泰语
        tr("Vietnamese"), // 越南语
    };

    // 避免Dialog关闭后，带起主窗口（~Qt::Dialog）。后来因为BUG-318107，增加Qt::Tool属性，弃用~Qt::Dialog。
    // 取消窗管的控制，避免置顶后影响MgmtWindow ChatWindow的行为
    this->setWindowFlags(this->windowFlags() | Qt::Tool | Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    this->initUi();
    this->initConnect();

    // 启动时，判断是否可写，一次性的。控制填充button的显示与否
    m_isWritable = WordWizard::fcitxWritable();
    // 第一次使用填充功能，提醒用户重启输入法服务
    m_isFirstFill = DConfigManager::instance()->value(AIQUICK_GROUP, AIQUICK_ISFIRSTFILL, true).toBool();
    if (m_isFirstFill) {
        m_isWritable = true;
    }
    if (kIsFirstTranslate) {
        // 避免每次新实例都获取，只获取一次
        kIsFirstTranslate = DConfigManager::instance()->value(AIQUICK_GROUP, AIQUICK_ISFIRSTTRANSLATE, true).toBool();
    }
}

AiQuickDialog::~AiQuickDialog()
{
    m_isQueryNeedShowTip = false;

    disconnect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &AiQuickDialog::onUpdateSystemTheme);
    disconnect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, &AiQuickDialog::onFontChanged);

    disconnect(EAiExec(), &EAiExecutor::uosAiLlmAccountLstChanged, this, &AiQuickDialog::onUosAiLlmAccountLstChanged);
    disconnect(EAiExec(), &EAiExecutor::llmAccountLstChanged, this, &AiQuickDialog::onLlmAccountLstChanged);
    disconnect(EAiExec(), &EAiExecutor::netStateChanged, this, &AiQuickDialog::onNetworkStateChanged);
    EAiExec()->clearAiRequest(this);

    disconnect(&FcitxInputServer::getInstance(), &FcitxInputServer::signalFocusIn, this, &AiQuickDialog::onFocusIn);
    disconnect(&FcitxInputServer::getInstance(), &FcitxInputServer::signalFocusOut, this, &AiQuickDialog::onFocusOut);

    disconnect(&WizardWrapper::instance(), &WizardWrapper::signalEscEvent, this, &AiQuickDialog::close);
}

void AiQuickDialog::initUi()
{
    this->setFixedWidth(450);
    this->setMaximumHeight(440);

    //标题栏
    m_titleBar = new DTitlebar(this);
    m_titleBar->setMenuVisible(false);
    m_titleBar->setBackgroundTransparent(true);
    m_titleBar->installEventFilter(this);

    m_closeBt = m_titleBar->findChild<DWindowCloseButton *>();
    if (m_closeBt) {
        DPalette closePalette = m_closeBt->palette();
        closePalette.setColor(DPalette::ButtonText, QColor(0, 0, 0, 178));
        m_closeBt->setPalette(closePalette);
    }

    m_logoBt = new TransButton(this);
    m_logoBt->setIcon(QIcon::fromTheme("uos-ai-assistant"));
    m_logoBt->setIconSize(QSize(25, 25));
    dynamic_cast<TransButton *>(m_logoBt)->setIconNoPressColor();

    m_titleBt = new TransButton(tr("type"), this);
    m_titleBt->setIcon(QIcon::fromTheme("uos-ai-assistant_aidropdown"));
    m_titleBt->setIconSize(QSize(10, 10));
    dynamic_cast<TransButton *>(m_titleBt)->setIconRight(true);
    dynamic_cast<TransButton *>(m_titleBt)->setNotAcceptFocus(true);
    m_titleBt->installEventFilter(this);
    DFontSizeManager::instance()->bind(m_titleBt, DFontSizeManager::T7, QFont::Normal);

    QHBoxLayout *allTitleLayout = new QHBoxLayout();
    allTitleLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    allTitleLayout->addSpacing(7);
    allTitleLayout->addWidget(m_logoBt);
    allTitleLayout->addSpacing(4);
    allTitleLayout->addWidget(m_titleBt);
    allTitleLayout->addWidget(m_titleBar);
    m_titleBt->setMaximumWidth(this->width() - m_logoBt->sizeHint().width() - 66);

    m_querySep = new DWidget(this);
    m_querySep->setFixedSize(2, 1);
    DPalette querySepPalette = m_querySep->palette();
    querySepPalette.setColor(DPalette::Window, QColor(0, 0, 0, 76));
    m_querySep->setPalette(querySepPalette);
    m_querySep->setAutoFillBackground(true);

    m_queryTextEdit = new QueryTextEdit(this);
    m_queryTextEdit->setContentsMargins(0, 0, 0, 0);
    m_queryTextEdit->setReadOnly(true);
    m_queryTextEdit->setFrameShape(QFrame::NoFrame);
    m_queryTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_queryTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_queryTextEdit->setWordWrapMode(QTextOption::WrapMode::WrapAnywhere);
    DPalette queryPalette = m_queryTextEdit->palette();
    queryPalette.setColor(DPalette::Base, QColor(255, 255, 255, 0));
    queryPalette.setColor(DPalette::Text, QColor(0, 0, 0, int(255 * 0.5)));
    m_queryTextEdit->setPalette(queryPalette);
    m_queryTextEdit->installEventFilter(this);
    m_queryTextEdit->setFixedHeight(1);

    m_queryTextEditMax = this->width() - 25 - m_querySep->width() - 4;
    m_queryTextEdit->setFixedWidth(m_queryTextEditMax);
    DFontSizeManager::instance()->bind(m_queryTextEdit, DFontSizeManager::T7, QFont::Normal);

    m_queryLayout = new QHBoxLayout();
    m_queryLayout->setContentsMargins(15, 0, 10, 0);
    m_queryLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_queryLayout->addWidget(m_querySep);
    m_queryLayout->addWidget(m_queryTextEdit);
    m_querySep->setVisible(true);
    m_queryTextEdit->setVisible(true);

    m_inOCRMovie = new QMovie(":assets/images/light/light-loading.gif");
    m_inOCRLabel = new DLabel(this);
    m_inOCRLabel->setMovie(m_inOCRMovie);
    // original size 800x83
    int inOCRWidth = this->width() - 30;
    float inOCRRatio = 800.0F / inOCRWidth;
    int inOCRHeight = 38; //static_cast<int>(83.0F / inProgressRatio);
    m_inOCRLabel->setFixedSize(inOCRWidth, inOCRHeight);
    m_inOCRLabel->setScaledContents(true);
    m_inOCRLabel->setVisible(false);
    m_queryLayout->addWidget(m_inOCRLabel);

    m_queryHSep = new DWidget(this);
    m_queryHSep->setFixedSize(this->width() - 30, 1);
    DPalette queryHSepPalette = m_queryHSep->palette();
    queryHSepPalette.setColor(DPalette::Window, QColor(0, 0, 0, 25));
    m_queryHSep->setPalette(queryHSepPalette);
    m_queryHSep->setAutoFillBackground(true);

    QHBoxLayout *queryHSepLayout = new QHBoxLayout();
    queryHSepLayout->setContentsMargins(15, 0, 15, 10);
    queryHSepLayout->addWidget(m_queryHSep);

    m_replyTextEdit = new WriterTextEdit(this);
    m_replyTextEdit->setFrameShape(QFrame::NoFrame);
    m_replyTextEdit->document()->setDocumentMargin(12);
    m_replyTextEdit->setReadOnly(true);
    //m_replyTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    DPalette replyPalette = m_replyTextEdit->palette();
    replyPalette.setColor(DPalette::Base, QColor(255, 255, 255, 0));
    replyPalette.setColor(DPalette::Text, QColor(0, 0, 0, 205));
    m_replyTextEdit->setPalette(replyPalette);
    m_replyTextEdit->setFixedHeight(0);
    DFontSizeManager::instance()->bind(m_replyTextEdit, DFontSizeManager::T7, QFont::Normal);

    QHBoxLayout *replyLayout = new QHBoxLayout();
    replyLayout->setContentsMargins(2, 0, 2, 0);
    replyLayout->setAlignment(Qt::AlignLeft);
    replyLayout->addWidget(m_replyTextEdit);

    m_errorInfoLabel = new DLabel(this);
    m_errorInfoLabel->setWordWrap(true);
    m_errorInfoLabel->setFixedWidth(this->width() - 30);
    m_errorInfoLabel->setContextMenuPolicy(Qt::NoContextMenu);
    DPalette errorPalette = m_errorInfoLabel->palette();
    errorPalette.setColor(DPalette::WindowText, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::Normal, DPalette::Text));
    m_errorInfoLabel->setPalette(errorPalette);
    m_errorInfoLabel->setVisible(false);

    m_cancelBt = new DPushButton(tr("Cancel"), this);
    m_cancelBt->setFixedHeight(20);
    m_cancelBt->setFixedWidth(m_cancelBt->sizeHint().width());
    DFontSizeManager::instance()->bind(m_cancelBt, DFontSizeManager::T8, QFont::Normal);
    m_cancelBt->setVisible(false);

    m_inProgressMovie = new QMovie(":assets/images/light/light-loading.gif");
    m_inProgressLabel = new DLabel(this);
    m_inProgressLabel->setMovie(m_inProgressMovie);
    // original size 800x83
    int inProgressWidth = this->width() - 30;
    float inProgressRatio = 800.0F / inProgressWidth;
    int inProgressHeight = 38; //static_cast<int>(83.0F / inProgressRatio);
    m_inProgressLabel->setFixedSize(inProgressWidth, inProgressHeight);
    m_inProgressLabel->setScaledContents(true);
    m_inProgressLabel->setVisible(false);

    QVBoxLayout *replyLayout1 = new QVBoxLayout();
    replyLayout1->setContentsMargins(15, 0, 15, 0);
    replyLayout1->setAlignment(Qt::AlignLeft);
    replyLayout1->addWidget(m_errorInfoLabel);
    replyLayout1->addWidget(m_cancelBt);
    replyLayout1->addWidget(m_inProgressLabel);

    // read
    m_readBt = new TransButton(tr("Read Aloud"), this);
    m_readBt->setIcon(QIcon::fromTheme("uos-ai-assistant_read"));
    m_readBt->setIconSize(QSize(16, 16));
    dynamic_cast<TransButton *>(m_readBt)->setNotAcceptFocus(true);
    DFontSizeManager::instance()->bind(m_readBt, DFontSizeManager::T8, QFont::Normal);

    // replace
    m_replaceBt = new TransButton(tr("Paste to Text"), this);
    m_replaceBt->setIcon(QIcon::fromTheme("uos-ai-assistant_replace"));
    m_replaceBt->setIconSize(QSize(16, 16));
    dynamic_cast<TransButton *>(m_replaceBt)->setNotAcceptFocus(true);
    DFontSizeManager::instance()->bind(m_replaceBt, DFontSizeManager::T8, QFont::Normal);

    m_replySep1 = new DWidget(this);
    m_replySep1->setFixedSize(1, 15);
    DPalette replyPalette1 = m_replySep1->palette();
    replyPalette1.setColor(DPalette::Window, QColor(0, 0, 0, 25));
    m_replySep1->setPalette(replyPalette1);
    m_replySep1->setAutoFillBackground(true);

    // again
    m_againBt = new TransButton(tr("Regenerate"), this);
    m_againBt->setIcon(QIcon::fromTheme("uos-ai-assistant_again"));
    m_againBt->setIconSize(QSize(16, 16));
    dynamic_cast<TransButton *>(m_againBt)->setNotAcceptFocus(true);
    DFontSizeManager::instance()->bind(m_againBt, DFontSizeManager::T8, QFont::Normal);

    // copy
    m_copyBt = new TransButton(tr("Copy"), this);
    m_copyBt->setIcon(QIcon::fromTheme("uos-ai-assistant_copy"));
    m_copyBt->setIconSize(QSize(16, 16));
    dynamic_cast<TransButton *>(m_copyBt)->setNotAcceptFocus(true);
    DFontSizeManager::instance()->bind(m_copyBt, DFontSizeManager::T8, QFont::Normal);

    m_replyFunLayout = new QHBoxLayout();
    m_replyFunLayout->setContentsMargins(10, 0, 10, 0);
    m_replyFunLayout->setAlignment(Qt::AlignLeft);
    m_replyFunLayout->addWidget(m_readBt);
    m_replyFunLayout->addWidget(m_replaceBt);
    //m_replyFunLayout->addSpacing(5);
    m_replyFunLayout->addWidget(m_replySep1);
    //m_replyFunLayout->addSpacing(5);
    m_replyFunLayout->addWidget(m_againBt);
    m_replyFunLayout->addWidget(m_copyBt);

    m_continueBt = new ContinueButton(this);
    m_continueBt->setText(tr("Continue Dialog"));
    m_continueBt->setIcon(QIcon::fromTheme("uos-ai-assistant_aisend"));
    m_continueBt->setIconSize(QSize(14, 14));
    m_continueBt->setFixedSize(426, 36);
    DFontSizeManager::instance()->bind(m_continueBt, DFontSizeManager::T7, QFont::Normal);
    m_continueBt->installEventFilter(this);

    QHBoxLayout *continueBtHLayout = new QHBoxLayout();
    continueBtHLayout->setContentsMargins(0, 0, 0, 0);
    continueBtHLayout->setAlignment(Qt::AlignCenter);
    continueBtHLayout->addWidget(m_continueBt);

    m_attentionLabel = new DLabel(this);
    m_attentionLabel->setText(tr("The content generated by AI is for reference only, please pay attention to the accuracy of the information."));
    m_attentionLabel->setWordWrap(true);
    QFont attentionFont = m_attentionLabel->font();
    attentionFont.setPixelSize(10);
    attentionFont.setWeight(QFont::Normal);
    m_attentionLabel->setFont(attentionFont);
    DPalette attentionPalette = m_attentionLabel->palette();
    attentionPalette.setColor(DPalette::WindowText, QColor(0, 0, 0, 102));
    m_attentionLabel->setPalette(attentionPalette);

    m_modelBt = new TransButton(this);
    m_modelBt->setIcon(QIcon::fromTheme("uos-ai-assistant_modelbt"));
    m_modelBt->setIconSize(QSize(16, 16));
    dynamic_cast<TransButton *>(m_modelBt)->setIconNoPressColor(true);
    m_modelBt->installEventFilter(this);
    m_attentionLabel->setFixedWidth(this->width() - 25 - m_modelBt->sizeHint().width());
    m_attentionLabel->setFixedHeight(m_attentionLabel->sizeHint().height());

    QHBoxLayout *attentionLayout = new QHBoxLayout();
    attentionLayout->setContentsMargins(10, 0, 10, 0);
    attentionLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    attentionLayout->addSpacing(5);
    attentionLayout->addWidget(m_attentionLabel);
    attentionLayout->addWidget(m_modelBt);

    m_vSpace = new DWidget(this);
    m_vSpace->setFixedHeight(4);

    m_vSpace1 = new DWidget(this);
    m_vSpace1->setFixedHeight(4);
    m_vSpace1->setVisible(false);

    m_vSpace2 = new DWidget(this);
    m_vSpace2->setFixedHeight(0);

    m_autoComboBox = new DComboBox(this);
    m_autoComboBox->addItem(tr("Automatic Detection"));
    m_autoComboBox->setFixedWidth(180);
    m_autoComboBox->setCurrentIndex(0);
    m_autoComboBox->setVisible(false);
    m_autoComboBox->setEnabled(false);
    m_autoComboBox->installEventFilter(this);

    // 语言选择下拉框
    m_languageComboBox = new DComboBox(this);
    m_languageComboBox->addItem("中文（简体）");
    m_languageComboBox->addItem("中文（繁體）");
    m_languageComboBox->addItem("བོད་སྐད་དང་།");
    m_languageComboBox->addItem("English");
    m_languageComboBox->addItem("日本語");
    m_languageComboBox->addItem("Deutsch");
    m_languageComboBox->addItem("Español");
    m_languageComboBox->addItem("Français");
    m_languageComboBox->addItem("Italiano");
    m_languageComboBox->addItem("한국인");
    m_languageComboBox->addItem("Bahasa Melayu");
    m_languageComboBox->addItem("português");
    m_languageComboBox->addItem("Русский");
    m_languageComboBox->addItem("ภาษาไทย");
    m_languageComboBox->addItem("Tiếng Việt");
    m_languageComboBox->setMaxVisibleItems(5);
    m_languageComboBox->setFixedWidth(180);
    m_languageComboBox->setCurrentIndex(0);
    m_languageComboBox->setVisible(false);

    m_languageComboBoxDelegate = new LanguageComboDelegate(this);
    // Set custom item delegate to show items with two lines: normal size and smaller size
    m_languageComboBox->setItemDelegate(m_languageComboBoxDelegate);
    m_languageComboBoxDelegate->setCurrentIndex(0);

    DPalette boxPalette = m_autoComboBox->palette();
    boxPalette.setColor(DPalette::ButtonText, QColor(0, 0, 0, int(255 * 0.7)));
    m_autoComboBox->setPalette(boxPalette);
    m_languageComboBox->setPalette(boxPalette);

    m_longArrowLabel = new DLabel(this);
    m_longArrowLabel->setPixmap(QIcon::fromTheme("uos-ai-assistant_longarrow").pixmap(QSize(34, 6)));
    m_longArrowLabel->setVisible(false);

    QHBoxLayout *languageLayout = new QHBoxLayout();
    languageLayout->setContentsMargins(15, 0, 15, 0);
    languageLayout->setAlignment(Qt::AlignCenter);
    languageLayout->addWidget(m_autoComboBox);
    languageLayout->addSpacing(10);
    languageLayout->addWidget(m_longArrowLabel);
    languageLayout->addSpacing(10);
    languageLayout->addWidget(m_languageComboBox);

    QVBoxLayout *allLayout = new QVBoxLayout(this);
    allLayout->setContentsMargins(0, 0, 0, 0);
    allLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    allLayout->addLayout(allTitleLayout);
    allLayout->addLayout(m_queryLayout);
    allLayout->addLayout(queryHSepLayout);
    allLayout->addLayout(languageLayout);
    allLayout->addWidget(m_vSpace1);
    allLayout->addLayout(replyLayout);
    allLayout->addLayout(replyLayout1);
    allLayout->addSpacing(8);
    allLayout->addLayout(m_replyFunLayout);
    allLayout->addWidget(m_vSpace);
    allLayout->addLayout(continueBtHLayout);
    allLayout->addLayout(attentionLayout);
    allLayout->addWidget(m_vSpace2);
    this->setLayout(allLayout);

    // 生成内容编辑框的高度有一个动态范围
    m_replyTextEditMax = this->maximumHeight() - this->sizeHint().height();
    qCDebug(logWordWizard) << "m_replyTextEditMax:" << m_replyTextEditMax;

    // menu
    m_menu = new CustomDMenu(this);
    m_searchAction = new QAction(tr("Search"), this);
    m_explainAction = new QAction(tr("Explain"), this);
    m_summarizeAction = new QAction(tr("Summary"), this);
    m_translateAction = new QAction(tr("Translate"), this);
    m_renewAction = new QAction(tr("Continue writing"), this);
    m_extendAction = new QAction(tr("Expand"), this);
    m_correctAction = new QAction(tr("Correct"), this);
    m_polishAction = new QAction(tr("Polish"), this);
    m_menu->addAction(m_searchAction);
    m_menu->addAction(m_explainAction);
    m_menu->addAction(m_summarizeAction);
    m_menu->addAction(m_translateAction);
    m_menu->addAction(m_renewAction);
    m_menu->addAction(m_extendAction);
    m_menu->addAction(m_correctAction);
    m_menu->addAction(m_polishAction);

    m_searchAction->setVisible(false);
    m_explainAction->setVisible(false);
    m_summarizeAction->setVisible(false);
    m_translateAction->setVisible(false);
    m_renewAction->setVisible(false);
    m_extendAction->setVisible(false);
    m_correctAction->setVisible(false);
    m_polishAction->setVisible(false);

#ifdef ENABLE_ASSISTANT
    m_knowledgeAction = new QAction(tr("Add to the AI knowledge base"), this);
    m_menu->addAction(m_knowledgeAction);
    m_knowledgeAction->setVisible(false);
#endif

    for (const CustomFunction &func : m_customFunctionList) {
        if (!func.isCustom) {
            if (func.isHidden) {
                continue;
            }

            switch (func.defaultFunctionType)
            {
            case WordWizard::WIZARD_TYPE_SEARCH:
                m_showActions.insert(m_searchAction);
                m_searchAction->setData(QVariant::fromValue(func));
                break;
            case WordWizard::WIZARD_TYPE_EXPLAIN:
                m_showActions.insert(m_explainAction);
                m_explainAction->setData(QVariant::fromValue(func));
                break;
            case WordWizard::WIZARD_TYPE_SUMMARIZE:
                m_showActions.insert(m_summarizeAction);
                m_summarizeAction->setData(QVariant::fromValue(func));
                break;
            case WordWizard::WIZARD_TYPE_TRANSLATE:
                m_showActions.insert(m_translateAction);
                m_translateAction->setData(QVariant::fromValue(func));
                break;
            case WordWizard::WIZARD_TYPE_RENEW:
                m_showActions.insert(m_renewAction);
                m_renewAction->setData(QVariant::fromValue(func));
                break;
            case WordWizard::WIZARD_TYPE_EXTEND:
                m_showActions.insert(m_extendAction);
                m_extendAction->setData(QVariant::fromValue(func));
                break;
            case WordWizard::WIZARD_TYPE_CORRECT:
                m_showActions.insert(m_correctAction);
                m_correctAction->setData(QVariant::fromValue(func));
                break;
            case WordWizard::WIZARD_TYPE_POLISH:
                m_showActions.insert(m_polishAction);
                m_polishAction->setData(QVariant::fromValue(func));
                break;
            case WordWizard::WIZARD_TYPE_KNOWLEDGE:
                if (m_knowledgeAction) {
                    m_showActions.insert(m_knowledgeAction);
                    m_knowledgeAction->setData(QVariant::fromValue(func));
                    updateKnowledgeActionEnabled();
                }
                break;
            default:
                break;
            }
        } else {
            QAction *action = new QAction(func.name, this);
            action->setData(QVariant::fromValue(func));
            m_showActions.insert(action);
            m_menu->addAction(action);
        }
    }

#ifdef COMPILE_ON_V25
    if (ESystemContext::isTreeland()) {
        qCInfo(logWordWizard) << "Is treeland, set role overlay";
        this->createWinId();
        DDEShellWayland::get(windowHandle())->setRole(QtWayland::treeland_dde_shell_surface_v1::role_overlay);
        DDEShellWayland::get(windowHandle())->setAcceptKeyboardFocus(false);
        DDEShellWayland::get(windowHandle())->setAutoPlacement(1);
    }
#endif
}

void AiQuickDialog::initConnect()
{
    if (m_parent) {
        connect(dynamic_cast<WordWizard *>(m_parent), &WordWizard::sigToLaunchAiQuick, this, &AiQuickDialog::close);
    }
    connect(m_logoBt, &DPushButton::clicked, this, &AiQuickDialog::onBtClicked);
    connect(m_titleBt, &DPushButton::clicked, this, &AiQuickDialog::onBtClicked);
    connect(m_readBt, &DPushButton::clicked, this, &AiQuickDialog::onBtClicked);
    connect(m_replaceBt, &DPushButton::clicked, this, &AiQuickDialog::onBtClicked);
    connect(m_againBt, &DPushButton::clicked, this, &AiQuickDialog::onBtClicked);
    connect(m_copyBt, &DPushButton::clicked, this, &AiQuickDialog::onBtClicked);
    connect(m_cancelBt, &DPushButton::clicked, this, &AiQuickDialog::onBtClicked);
    connect(m_continueBt, &DPushButton::clicked, this, &AiQuickDialog::onBtClicked);

    connect(m_errorInfoLabel, &DLabel::linkActivated, this, &AiQuickDialog::onOpenConfigDialog);

    connect(m_menu, &QMenu::triggered, this, &AiQuickDialog::onMenuTriggered);

    connect(m_languageComboBox, QOverload<int>::of(&DComboBox::currentIndexChanged), this, [&] (int index) {
        if (kTargetLanguageIdx == index) {
            return;
        }

        kTargetLanguageIdx = index;
        m_languageComboBoxDelegate->setCurrentIndex(index);
        DConfigManager::instance()->setValue(AIQUICK_GROUP, AIQUICK_TRANSLATE_TARGET_LANGUAGE, kTargetLanguageIdx);
        if (kTargetLanguageIdx >= 0 && kTargetLanguageIdx < kLanguageList.size()) {
            if (kIsFirstTranslate && kTargetLanguageIdx != LANGUAGE_TYPE_SIMPLIFIED_CHINESE && kTargetLanguageIdx != LANGUAGE_TYPE_ENGLISH) {
                kIsFirstTranslate = false;
                DConfigManager::instance()->setValue(AIQUICK_GROUP, AIQUICK_ISFIRSTTRANSLATE, kIsFirstTranslate);
                TopDialog dlg(this);
                dlg.setMinimumWidth(380);
                dlg.setIcon(QIcon(WARNING_ICON));
                dlg.setMessage(tr("The translation result is limited by the model's capabilities. If you are not satisfied with the translation, please switch to other models on the UOS AI Home page."));
                dlg.addButton(tr("Ok"), true, DDialog::ButtonNormal);
                dlg.exec();
            }
        } else {
            kTargetLanguageIdx = LANGUAGE_TYPE_SIMPLIFIED_CHINESE;
        }
        m_systemPrompt = QString(tr("Translate this passage into %1 and give me a clear result directly.\ntext：")).arg(m_languagePromptList[kTargetLanguageIdx]) + m_query;
        this->sendAiRequst();
    });

    connect(m_replyTextEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [&] (int value) {
        if (!m_isReplyEditCursorEnd && value == m_replyTextEdit->verticalScrollBar()->maximum()) {
            m_isReplyEditCursorEnd = true;
        } else if (m_isReplyEditCursorEnd && m_replyTextEdit->verticalScrollBar()->maximum() - value > 50) {
            m_isReplyEditCursorEnd = false;
        }
    });

    // theme
    this->onUpdateSystemTheme();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &AiQuickDialog::onUpdateSystemTheme);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, &AiQuickDialog::onFontChanged);

    connect(EAiExec(), &EAiExecutor::uosAiLlmAccountLstChanged, this, &AiQuickDialog::onUosAiLlmAccountLstChanged);
    connect(EAiExec(), &EAiExecutor::llmAccountLstChanged, this, &AiQuickDialog::onLlmAccountLstChanged);
    connect(EAiExec(), &EAiExecutor::netStateChanged, this, &AiQuickDialog::onNetworkStateChanged);
    connect(EAiExec(), &EAiExecutor::sigWebViewLoadFinished, this, &AiQuickDialog::onWebViewLoadFinished);

    // 依赖fcitx：当前可写状态
    connect(&FcitxInputServer::getInstance(), &FcitxInputServer::signalFocusIn, this, &AiQuickDialog::onFocusIn);
    connect(&FcitxInputServer::getInstance(), &FcitxInputServer::signalFocusOut, this, &AiQuickDialog::onFocusOut);

    // 使用划词工具栏的键盘监听信号
    connect(&WizardWrapper::instance(), &WizardWrapper::signalEscEvent, this, &AiQuickDialog::close);
}

void AiQuickDialog::setQuery(int type, QString query, QPoint pos, bool isCustom, const QString &imagePath)
{
    qCInfo(logWordWizard) << "setQuery - type:" << type << "query:" << query << "pos:" << pos << "isCustom:" << isCustom;
    m_queryType = type;
    m_query     = query;
    m_pos       = pos;

    // layout
    if (!ESystemContext::isTreeland()) {
        if (true) {
            int x = m_pos.x() - this->width() / 2;
            int y = m_pos.y();

            QList<QScreen *> screens = QGuiApplication::screens();
            for(QScreen *screen : screens) {
                if (screen->geometry().contains(m_pos.x(), m_pos.y())) {
                    int leftLimit = screen->geometry().left() + 10;
                    if (x < leftLimit) {
                        x = leftLimit;
                    }
                    int rightLimit = screen->geometry().right() - 10 - this->width();
                    if (x > rightLimit) {
                        x = rightLimit;
                    }
                    qCDebug(logWordWizard) << "screen:" << screen->geometry() << "pos:" << QPoint(x, y);
                    break;
                }
            }

            this->move(x, y);
        } else {
            this->move(m_pos);
        }
    }

    // type
    if (!isCustom) {
        // 重定向type，实现统一化
        m_queryType = WordWizard::WIZARD_TYPE_EXPLAIN;
        for (int i = 0; i < m_customFunctionList.size(); i++) {
            if (!m_customFunctionList[i].isCustom && m_customFunctionList[i].defaultFunctionType == type) {
                m_queryType = i;
                break;
            }
        }
    }
    const CustomFunction &func = m_customFunctionList[m_queryType];
    if (!func.isCustom) {
        qCInfo(logWordWizard) << "Default function type:" << func.defaultFunctionType;
        this->setQueryType(func.defaultFunctionType);
    } else {
        qCInfo(logWordWizard) << "Custom function name:" << func.name;
        this->setCustomQueryType(func);
    }
    updateKnowledgeActionEnabled();
    this->syncLlmAccount();

    if(imagePath != "") {
       runOCRProcessByPath(type, isCustom, imagePath);
    } else {
        m_queryTextEdit->setFullText(m_query);
        // query
        m_query.replace("￼", " ");
        this->sendAiRequst();
    }
}

void AiQuickDialog::showEvent(QShowEvent *event)
{
    qCDebug(logWordWizard) << "Showing quick dialog";
    DAbstractDialog::showEvent(event);
    this->asyncAdjustSize(100);
}

void AiQuickDialog::closeEvent(QCloseEvent *event)
{
    qCDebug(logWordWizard) << "Closing quick dialog";
    EAiExec()->cancelAiRequst(m_reqId);
    this->deleteLater();
    return DAbstractDialog::closeEvent(event);
}

void AiQuickDialog::reject()
{
    this->close();
    return DAbstractDialog::reject();
}

bool AiQuickDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_queryTextEdit) {
        if (event->type() == QEvent::Enter) { // 配置了nofocus窗口属性后，需要手动弹出tooltip
            // 优化体验，延迟显示tooltip
            m_isQueryNeedShowTip = true;
            /*QTimer::singleShot(1000, this, [&]{
                if (!m_isQueryNeedShowTip) {
                    return;
                }

                QToolTip::showText(QCursor::pos(), m_queryTextEdit->toolTip());
            });*/
            return true;
        } else if (event->type() == QEvent::Leave) {
            m_isQueryNeedShowTip = false;
            return true;
        } else if (event->type() == QEvent::Timer) { // 避免点击滑动时文本被编辑框隐藏
            return true;
        }
    } else if (watched == m_titleBt) {
        static bool isNeedShowTip = true;
        if (event->type() == QEvent::Enter) { // 配置了nofocus窗口属性后，需要手动弹出tooltip
            isNeedShowTip = true;
            QTimer::singleShot(1000, this, [&] {
                if (!isNeedShowTip && m_titleBt) {
                    return;
                }

                QToolTip::showText(QCursor::pos(), m_titleBt->toolTip(), m_titleBt);
            });
            return false;
        } else if (event->type() == QEvent::Leave) {
            isNeedShowTip = false;
            return false;
        }
    } else if (watched == m_modelBt) {
        static bool isNeedShowTip = true;
        if (event->type() == QEvent::Enter) { // 配置了nofocus窗口属性后，需要手动弹出tooltip
            isNeedShowTip = true;
            QTimer::singleShot(1000, this, [&] {
                if (!isNeedShowTip && m_modelBt) {
                    return;
                }

                QToolTip::showText(QCursor::pos(), m_modelInfo, m_modelBt);
            });
            return true;
        } else if (event->type() == QEvent::Leave) {
            isNeedShowTip = false;
            return true;
        }
    } else if (watched == m_autoComboBox) {
        static bool isNeedShowTip = true;
        if (event->type() == QEvent::Enter) { // 配置了nofocus窗口属性后，需要手动弹出tooltip
            isNeedShowTip = true;
            QTimer::singleShot(1000, this, [&] {
                if (!isNeedShowTip && m_autoComboBox) {
                    return;
                }

                QToolTip::showText(QCursor::pos(), tr("Not clickable"), m_autoComboBox);
            });
            return true;
        } else if (event->type() == QEvent::Leave) {
            isNeedShowTip = false;
            return true;
        }
    } else if (watched == m_titleBar) {
        if (!ESystemContext::isTreeland()) {
            // 取消窗管控制后，自己实现拖拽行为
            if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseMove) {
                QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(event);
                if (event->type() == QEvent::MouseButtonPress) {
                    this->mousePressEvent(mouseEvent);
                } else if (event->type() == QEvent::MouseButtonRelease) {
                    this->mouseReleaseEvent(mouseEvent);
                } else if (event->type() == QEvent::MouseMove) {
                    this->mouseMoveEvent(mouseEvent);
                }
            }
        }
    }

    return DAbstractDialog::eventFilter(watched, event);
}

void AiQuickDialog::mousePressEvent(QMouseEvent *event)
{
    if (!ESystemContext::isTreeland()) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            return;
        }
    }

    DAbstractDialog::mousePressEvent(event);
}

void AiQuickDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (!ESystemContext::isTreeland()) {
        if (m_dragging) {
            this->setCursor(Qt::CursorShape::SizeAllCursor);
            this->move(event->globalPos() - m_dragStartPos);
            return;
        }
    }

    DAbstractDialog::mouseMoveEvent(event);
}

void AiQuickDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (!ESystemContext::isTreeland()) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            this->setCursor(Qt::CursorShape::ArrowCursor);
            return;
        }
    }

    DAbstractDialog::mouseReleaseEvent(event);
}

void AiQuickDialog::onBtClicked()
{
    if (sender() == m_logoBt) {
        //emit ServerWrapper::instance()->sigToLaunchChat(ChatIndex::Text);
        WordWizard::onIconBtnClicked(m_query);
        return;
    }

    if (sender() == m_titleBt) {
        m_menu->move(QCursor::pos());
        m_menu->show();
        return;
    }

    if (sender() == m_readBt) {
        if (!EAiExec()->isAudioOutputAvailable()) {
            // 未检测到音频输出设备，请检查后重试
            if (true) {
                this->showToast(tr("The audio device is not detected, please check and try again."));
            } else {
                TopDialog dlg(this);
                dlg.setMinimumWidth(380);
                dlg.setIcon(QIcon(":assets/images/tips.svg"));
                dlg.setMessage(tr("The audio device is not detected, please check and try again."));
                dlg.addButton(tr("Ok", "button"), true, DDialog::ButtonNormal);
                dlg.exec();
            }
            return;
        }

        BaseClipboard::ttsInstance()->setClipText(m_reply);
        QDBusInterface notification("com.iflytek.aiassistant", "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
        notification.call(QDBus::NoBlock, "TextToSpeech");
        return;
    }

    if (sender() == m_replaceBt) {
        qCInfo(logWordWizard) << "Replace button clicked, isFirstFill:" << m_isFirstFill
                            << "isWritable:" << m_isWritable;
        if (m_isFirstFill) { // 第一次使用填充功能，提醒用户重启输入法服务
            TopDialog dlg(this);
            dlg.setMinimumWidth(380);
            dlg.setIcon(QIcon(":assets/images/tips.svg"));
            dlg.setMessage(tr("The function can be used only after restarting the input method. The tutorial is as follows:\n1. Right-click on the input method icon in the taskbar.\n2. Click \"Restart\"."));
            DLabel imageLabel(this);
            QString iconName = "uos-ai-assistant_fcitxrestart";
            if (!Util::checkLanguage())
                iconName = "uos-ai-assistant_fcitxrestart_en";
            imageLabel.setPixmap(QPixmap(QIcon::fromTheme(iconName).pixmap(160, 160)));
            dlg.addContent(&imageLabel, Qt::AlignCenter);
            dlg.addButton(tr("Ok", "button"), true, DDialog::ButtonNormal);
            connect(&dlg, &DDialog::finished, this, [&] {
                qCDebug(logWordWizard) << "fcitx restart accepted";
                m_isFirstFill = false;
                DConfigManager::instance()->setValue(AIQUICK_GROUP, AIQUICK_ISFIRSTFILL, false);
            });
            dlg.exec();
        } else if (!WordWizard::fcitxWritable()) { // 此时，再次判断是否可写
            TopDialog dlg(this);
            dlg.setFixedWidth(380);
            dlg.setIcon(QIcon(WARNING_ICON));
            dlg.setTitle(tr("Fill failed"));
            dlg.setMessage(tr("No input box selected, please select the input box and re-fill it."));
            dlg.addButton(tr("Ok"), true, DDialog::ButtonNormal);
            connect(&dlg, &DDialog::finished, this, [&] {
                qCDebug(logWordWizard) << "fill accepted";
            });
            dlg.move(this->geometry().x() + (this->width() - dlg.width()) / 2, this->geometry().y() + (this->height() - dlg.height()) / 2);
            dlg.exec();
        } else {
            FcitxInputServer::getInstance().commitString(m_reply);
        }
        return;
    }

    if (sender() == m_againBt) {
        sendAiRequst();
        return;
    }

    if (sender() == m_copyBt) {
        this->copyText(m_reply);
        return;
    }

    if (sender() == m_cancelBt) {
        m_curErr = AiQuickDialog::ERROR_TYPE_NONE;
        this->enableReplyFunBt(true);

        m_reply = m_replyBak;
        m_replyTextEdit->setPlainText(m_reply);
        m_replyTextEdit->setVisible(true);
        m_errorInfoLabel->setVisible(false);
        m_cancelBt->setVisible(false);

        if (m_queryType != m_queryTypeBak) {
            m_queryType = m_queryTypeBak;
            const CustomFunction &func = m_customFunctionList[m_queryType];
            if (!func.isCustom) {
                qCInfo(logWordWizard) << "Default function type:" << func.defaultFunctionType;
                this->setQueryType(func.defaultFunctionType);
            } else {
                qCInfo(logWordWizard) << "Custom function name:" << func.name;
                this->setCustomQueryType(func);
            }
        }

        this->asyncAdjustSize();
        return;
    }

    if (sender() == m_continueBt) {
        this->continueDialog();
        return;
    }
}

void AiQuickDialog::onMenuTriggered(QAction *action)
{
    if (action == m_searchAction) {
        WordWizard::onSearchBtnClicked(m_query);
        return;
    }

    if (action == m_knowledgeAction) {
        close();
        if (WordWizard::doAddToKnowledgeBase(m_query))
            showToast(tr("Added"));
        return;
    }

    const CustomFunction &func = action->data().value<CustomFunction>();
    for (int i = 0; i < m_customFunctionList.size(); i++) {
        if (m_customFunctionList[i] == func) {
            m_queryType = i;
            break;
        }
    }
    if (!func.isCustom) {
        qCInfo(logWordWizard) << "Default function type:" << func.defaultFunctionType;
        this->setQueryType(func.defaultFunctionType);
    } else {
        qCInfo(logWordWizard) << "Custom function name:" << func.name;
        this->setCustomQueryType(func);
    }

    this->sendAiRequst();
    this->asyncAdjustSize();
}

void AiQuickDialog::onModelReply(int op, QString reply, int err)
{
    /*
    {
        "message": {
            "chatType": 0,
            "content": "xxx"
        },
        "stream": true
    }
    */
    if (!reply.isEmpty() && reply.startsWith("{")) {
        QJsonObject rootJson = QJsonDocument::fromJson(reply.toUtf8()).object();
        QJsonObject msgJson = rootJson.value("message").toObject();
        int chatType = msgJson.value("chatType").toInt(ChatAction::ChatTextPlain);
        reply = msgJson.value("content").toString();
        if (chatType != ChatAction::ChatTextPlain) {
            return;
        }
    }

    if (err != AIServer::NoError && err != 200) {
        qCWarning(logWordWizard) << "Model reply error:" << err << "reply:" << reply;
    }

    m_replyTextEdit->setVisible(true);
    m_inProgressLabel->setVisible(false);
    m_inProgressMovie->stop();

    /**
     * FREEACCOUNTEXPIRED = 9000,
     * FREEACCOUNTUSAGELIMIT = 9001,
     * FREEACCOUNTCHATUSAGELIMIT = 9002,
     * FREEACCOUNTTEXT2IMAGEUSAGELIMIT = 9003,
     * AccountInvalid = 9004,
     */
    if (err == AIServer::NoError) {
        m_modelErrCode = err;
        m_curErr = ERROR_TYPE_NONE;
        m_isReplyEnd = false;
        this->putAiReply(reply);
    } else if (err == 200) { // 流式回答结束符号
        m_modelErrCode = err;
        m_isReplyEnd = true;
        this->putAiReply();
        // 记录此次成功的请求类型
        m_queryTypeBak = m_queryType;
        // 记录此次成功的请求模型
        m_modelIdBak   = m_modelId;
        m_modelNameBak = m_modelName;
        m_modelIconBak = m_modelIcon;
    } else { // ERROR part
        m_isReplyEnd = true;
        this->setReplyPartVisible(true);
        err = std::abs(err);
        if (err == AIServer::FREEACCOUNTUSAGELIMIT || err == AIServer::FREEACCOUNTCHATUSAGELIMIT || err == AIServer::FREEACCOUNTTEXT2IMAGEUSAGELIMIT) {  // 额度用完
            m_curErr     = AiQuickDialog::ERROR_TYPE_ACCOUNT_LIMIT;
            m_curErrInfo = reply;
            this->handleError();
        } else if (err == AIServer::FREEACCOUNTEXPIRED) {  // 账号过期
            m_curErr     = AiQuickDialog::ERROR_TYPE_ACCOUNT_EXPIRED;
            m_curErrInfo = reply;
            this->handleError();
        } else if (err == AIServer::AccountInvalid) {  // 账号无效
            m_curErr     = AiQuickDialog::ERROR_TYPE_ACCOUNT_INVALID;
            m_curErrInfo = reply;
            this->handleError();
        } else {
            m_modelErrCode = -err;
            // 其他错误的兜底处理，测试说：按正常回答处理
            m_curErr = AiQuickDialog::ERROR_TYPE_NONE;
            this->putAiReply(reply);
            // 记录此次成功的请求类型
            m_queryTypeBak = m_queryType;
            // 记录此次成功的请求模型
            m_modelIdBak   = m_modelId;
            m_modelNameBak = m_modelName;
            m_modelIconBak = m_modelIcon;
        }
    }

    this->asyncAdjustSize();
}

void AiQuickDialog::onUpdateSystemTheme()
{
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (m_themeType == themeType) {
        return;
    }

    if (themeType == DGuiApplicationHelper::LightType) {
        DPalette titlePalette = m_titleBt->palette();
        titlePalette.setColor(DPalette::ButtonText, QColor(0, 0, 0, 178));
        m_titleBt->setPalette(titlePalette);

        if (m_closeBt) {
            DPalette closePalette = m_closeBt->palette();
            closePalette.setColor(DPalette::ButtonText, QColor(0, 0, 0, 178));
            m_closeBt->setPalette(closePalette);
        }

        DPalette querySepPalette = m_querySep->palette();
        querySepPalette.setColor(DPalette::Window, QColor(0, 0, 0, 76));
        m_querySep->setPalette(querySepPalette);

        DPalette queryHSepPalette = m_queryHSep->palette();
        queryHSepPalette.setColor(DPalette::Window, QColor(0, 0, 0, 25));
        m_queryHSep->setPalette(queryHSepPalette);

        DPalette queryPalette = m_queryTextEdit->palette();
        queryPalette.setColor(DPalette::Text, QColor(0, 0, 0, int(255 * 0.5)));
        m_queryTextEdit->setPalette(queryPalette);

        DPalette boxPalette = m_autoComboBox->palette();
        boxPalette.setColor(DPalette::ButtonText, QColor(0, 0, 0, int(255 * 0.7)));
        m_autoComboBox->setPalette(boxPalette);
        m_languageComboBox->setPalette(boxPalette);

        m_longArrowLabel->setPixmap(QIcon::fromTheme("uos-ai-assistant_longarrow").pixmap(QSize(34, 6)));

        DPalette replyPalette = m_replyTextEdit->palette();
        replyPalette.setColor(DPalette::Base, QColor(255, 255, 255, 0));
        replyPalette.setColor(DPalette::Text, QColor(0, 0, 0, 205));
        m_replyTextEdit->setPalette(replyPalette);

        DPalette errorPalette = m_errorInfoLabel->palette();
        errorPalette.setColor(DPalette::WindowText, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::Normal, DPalette::Text));
        m_errorInfoLabel->setPalette(errorPalette);

        m_cancelBt->setPalette(titlePalette);

        DPalette replyPalette1 = m_replySep1->palette();
        replyPalette1.setColor(DPalette::Window, QColor(0, 0, 0, 25));
        m_replySep1->setPalette(replyPalette1);

        DPalette attentionPalette = m_attentionLabel->palette();
        attentionPalette.setColor(DPalette::WindowText, QColor(0, 0, 0, 102));
        m_attentionLabel->setPalette(attentionPalette);

        m_inProgressMovie->setFileName(":assets/images/light/light-loading.gif");
        if (m_inProgressMovie->state() == QMovie::Running) {
            m_inProgressMovie->stop();
            m_inProgressMovie->start();
        }
        m_inOCRMovie->setFileName(":assets/images/light/light-loading.gif");
        if (m_inOCRMovie->state() == QMovie::Running) {
            m_inOCRMovie->stop();
            m_inOCRMovie->start();
        }
    } else {
        DPalette titlePalette = m_titleBt->palette();
        titlePalette.setColor(DPalette::ButtonText, QColor(255, 255, 255, 178));
        m_titleBt->setPalette(titlePalette);

        if (m_closeBt) {
            DPalette closePalette = m_closeBt->palette();
            closePalette.setColor(DPalette::ButtonText, QColor(255, 255, 255, 178));
            m_closeBt->setPalette(closePalette);
        }

        DPalette querySepPalette = m_querySep->palette();
        querySepPalette.setColor(DPalette::Window, QColor(255, 255, 255, 76));
        m_querySep->setPalette(querySepPalette);

        DPalette queryHSepPalette = m_queryHSep->palette();
        queryHSepPalette.setColor(DPalette::Window, QColor(255, 255, 255, 25));
        m_queryHSep->setPalette(queryHSepPalette);

        DPalette queryPalette = m_queryTextEdit->palette();
        queryPalette.setColor(DPalette::Text, QColor(255, 255, 255, 153));
        m_queryTextEdit->setPalette(queryPalette);

        DPalette boxPalette = m_autoComboBox->palette();
        boxPalette.setColor(DPalette::ButtonText, QColor(255, 255, 255, int(255 * 0.7)));
        m_autoComboBox->setPalette(boxPalette);
        m_languageComboBox->setPalette(boxPalette);

        m_longArrowLabel->setPixmap(QIcon::fromTheme("uos-ai-assistant_longarrow").pixmap(QSize(34, 6)));

        DPalette replyPalette = m_replyTextEdit->palette();
        replyPalette.setColor(DPalette::Base, QColor(0, 0, 0, 0));
        replyPalette.setColor(DPalette::Text, QColor(255, 255, 255, 205));
        m_replyTextEdit->setPalette(replyPalette);

        DPalette errorPalette = m_errorInfoLabel->palette();
        errorPalette.setColor(DPalette::WindowText, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::Normal, DPalette::Text));
        m_errorInfoLabel->setPalette(errorPalette);

        m_cancelBt->setPalette(titlePalette);

        DPalette replyPalette1 = m_replySep1->palette();
        replyPalette1.setColor(DPalette::Window, QColor(255, 255, 255, 25));
        m_replySep1->setPalette(replyPalette1);

        DPalette attentionPalette = m_attentionLabel->palette();
        attentionPalette.setColor(DPalette::WindowText, QColor(255, 255, 255, 102));
        m_attentionLabel->setPalette(attentionPalette);

        m_inProgressMovie->setFileName(":assets/images/dark/dark-loading.gif");
        if (m_inProgressMovie->state() == QMovie::Running) {
            m_inProgressMovie->stop();
            m_inProgressMovie->start();
        }
        m_inOCRMovie->setFileName(":assets/images/light/dark-loading.gif");
        if (m_inOCRMovie->state() == QMovie::Running) {
            m_inOCRMovie->stop();
            m_inOCRMovie->start();
        }
    }
    m_themeType = themeType;
    update();
}

void AiQuickDialog::onFontChanged(const QFont &font)
{
    this->asyncAdjustSize();
}

void AiQuickDialog::onUosAiLlmAccountLstChanged()
{
    qCInfo(logWordWizard) << "Llm account list changed";
    QString modelIdOrig = m_modelId;
    this->syncLlmAccount();
    // 账号列表变化了，但是首选model没变
    if (m_modelId == modelIdOrig) {
        return;
    }

    if (m_curErr != ERROR_TYPE_NONE) {
        m_errorInfoLabel->setVisible(false);
        m_cancelBt->setVisible(false);

        this->sendAiRequst();
    }
}

void AiQuickDialog::onLlmAccountLstChanged(const QString &currentAccountId, const QString &accountLst)
{
    this->onUosAiLlmAccountLstChanged();
}

void AiQuickDialog::onOpenConfigDialog(const QString& link)
{
    EAiExec()->launchLLMConfigWindow(false, false, false, tr("Model Configuration"));
    // BUG-292967（面板脱离窗管置顶，遮住了设置窗口）  打开设置窗口后，关闭面板
    this->close();
}

void AiQuickDialog::onNetworkStateChanged(bool isOnline)
{
    this->onUosAiLlmAccountLstChanged();
}

void AiQuickDialog::onWritableStateChanged(bool isTrue)
{
    if (false) { // 产品计划下一阶段，再实施动态显示
        m_isWritable = isTrue;
        if (m_continueBt->isVisible()) {
            m_replaceBt->setVisible(m_isWritable);
            m_replySep1->setVisible(m_isWritable);
        }
    }
}

void AiQuickDialog::showToast(const QString &message)
{
    DFloatingMessage *floatMessage = new DFloatingMessage(DFloatingMessage::TransientType, this);
    floatMessage->setMessage(message);
    floatMessage->setAttribute(Qt::WA_DeleteOnClose);
    DIconButton *bt = floatMessage->findChild<DIconButton *>();
    if (bt) {
        bt->setVisible(false);
    }
    QLabel *label = floatMessage->findChild<QLabel *>();
    if (label) {
        label->setAlignment(Qt::AlignCenter);
        DPalette labelPalette = label->palette();
        labelPalette.setColor(DPalette::WindowText, QColor(0, 0, 0, 178));
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
            labelPalette.setColor(DPalette::WindowText, QColor(255, 255, 255, 178));
        }
        label->setPalette(labelPalette);
    }

    QRect geometry(QPoint(10, 10), floatMessage->sizeHint());
    geometry.moveCenter(rect().center());

    //Show the message near bottom
    geometry.moveBottom(rect().bottom() - 8);
    floatMessage->setGeometry(geometry);
    floatMessage->show();
}

void AiQuickDialog::copyText(const QString &text)
{
    this->showToast(tr("Copied"));
    QApplication::clipboard()->setText(text);
}

void AiQuickDialog::setQuerySepHeight(int height)
{
    m_querySep->setFixedHeight(height);
}

void AiQuickDialog::setQueryType(int type)
{
    for (QAction *action : m_showActions) {
        action->setVisible(true);
    }

    m_readBt->setVisible(false);
    m_autoComboBox->setVisible(false);
    m_languageComboBox->setVisible(false);
    m_longArrowLabel->setVisible(false);
    m_vSpace1->setVisible(false);

    switch (type) {
    case WordWizard::WIZARD_TYPE_EXPLAIN:
        m_menuLabel = tr("Explain");
        m_explainAction->setVisible(false);
        m_systemPrompt = tr("Explain this passage in plain language. Just give me a clear result without redundant content.\ntext：");
        break;
    case WordWizard::WIZARD_TYPE_SUMMARIZE:
        m_menuLabel = tr("Summary");
        m_summarizeAction->setVisible(false);
        m_systemPrompt = tr("Summarize this passage and give me a clear result directly without any other redundant content.\ntext：");
        break;
    case WordWizard::WIZARD_TYPE_TRANSLATE:
        m_menuLabel = tr("Translate");
        m_translateAction->setVisible(false);
        m_autoComboBox->setVisible(true);
        m_languageComboBox->setVisible(true);
        m_longArrowLabel->setVisible(true);
        m_vSpace1->setVisible(true);
        // 默认选中项
        if (kTargetLanguageIdx == -1) {
            kTargetLanguageIdx = DConfigManager::instance()->value(AIQUICK_GROUP, AIQUICK_TRANSLATE_TARGET_LANGUAGE, LANGUAGE_TYPE_SIMPLIFIED_CHINESE).toInt();
        }
        if (!(kTargetLanguageIdx >= 0 && kTargetLanguageIdx < kLanguageList.size())) {
            kTargetLanguageIdx = LANGUAGE_TYPE_SIMPLIFIED_CHINESE;
        }
        m_readBt->setVisible(isReadAloudSupported());
        m_languageComboBox->setCurrentIndex(kTargetLanguageIdx);
        m_languageComboBoxDelegate->setCurrentIndex(kTargetLanguageIdx);
        m_systemPrompt = QString(tr("Translate this passage into %1 and give me a clear result directly.\ntext：")).arg(m_languagePromptList[kTargetLanguageIdx]);
        break;
    case WordWizard::WIZARD_TYPE_RENEW:
        m_menuLabel = tr("Continue writing");
        m_renewAction->setVisible(false);
        m_systemPrompt = tr("Continue this passage appropriately. No need for a lot of words. Just give me a clear result without any other redundant content.\ntext：");
        break;
    case WordWizard::WIZARD_TYPE_EXTEND:
        m_menuLabel = tr("Expand");
        m_extendAction->setVisible(false);
        m_systemPrompt = tr("Expand this passage appropriately. No need for a particularly long text. Just give me a clear result without any other redundant content.\ntext：");
        break;
    case WordWizard::WIZARD_TYPE_CORRECT:
        m_menuLabel = tr("Correct");
        m_correctAction->setVisible(false);
        m_systemPrompt = tr("Check if there are any typos in this passage. If there are, point out the location of the errors. Give me a clear result directly without any other redundant content.\ntext：");
        break;
    case WordWizard::WIZARD_TYPE_POLISH:
        m_menuLabel = tr("Polish");
        m_polishAction->setVisible(false);
        m_systemPrompt = tr("Polish this passage for me and just give me the result without any extra content.\ntext：");
        break;
    default:
        m_menuLabel = tr("Explain");
        m_explainAction->setVisible(false);
        m_systemPrompt = "";
        break;
    }
    dynamic_cast<TransButton *>(m_titleBt)->changeText(m_menuLabel);
    m_titleBt->setToolTip(m_menuLabel);
    m_systemPrompt += m_query;

    m_replyTextEdit->setVisible(true);
    m_errorInfoLabel->setVisible(false);
    m_cancelBt->setVisible(false);

    this->asyncAdjustSize();
}

void AiQuickDialog::setCustomQueryType(const CustomFunction &func)
{
    for (QAction *action : m_showActions) {
        action->setVisible(true);
    }

    m_readBt->setVisible(false);
    m_autoComboBox->setVisible(false);
    m_languageComboBox->setVisible(false);
    m_longArrowLabel->setVisible(false);
    m_vSpace1->setVisible(false);

    for (QAction *action : m_showActions) {
        if (action->data().value<CustomFunction>().name == func.name) {
            m_menuLabel = func.name;
            dynamic_cast<TransButton *>(m_titleBt)->changeText(m_menuLabel);
            m_titleBt->setToolTip(m_menuLabel);
            action->setVisible(false);

            m_systemPrompt = func.prompt;
            // 占位符替换
            if (m_systemPrompt.contains(CustomFunction::kPlaceholder)) {
                m_systemPrompt.replace(CustomFunction::kPlaceholder, QString(tr(" \"%1\" ")).arg(m_query));
            } else {
                m_systemPrompt += QString(tr(" \"%1\" ")).arg(m_query);
            }
            break;
        }
    }

    m_replyTextEdit->setVisible(true);
    m_errorInfoLabel->setVisible(false);
    m_cancelBt->setVisible(false);

    this->asyncAdjustSize();
}

void AiQuickDialog::sendAiRequst()
{
    // AI model
    // [{\"description\":\"Welcome to UOS AI, the comprehensive assistant of UOS AI system.\",\"displayname\":\"UOS AI\",\"icon\":\"uos-ai\",\"iconPrefix\":\"icons/\",\"id\":\"uos-ai_1723016869400\",\"type\":1},{\"description\":\"UOS System Assistant, answers questions related to using the UOS operating system.\",\"displayname\":\"UOS System Assistant\",\"icon\":\"system-assistant\",\"iconPrefix\":\"icons/\",\"id\":\"uos-system-assistant_1723016869400\",\"type\":2},{\"description\":\"A personal knowledge assistant who can answer questions and generate content based on your personal file data. You can add or delete knowledge on the Settings - Knowledge Base Management page.\",\"displayname\":\"Personal Knowledge Assistant\",\"icon\":\"personal-assistant\",\"iconPrefix\":\"icons/\",\"id\":\"personal-knowledge-assistant_1723016869400\",\"type\":4}]
    if (false) {
        QString assisListJson = EAiExec()->queryAssistantList();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(assisListJson.toUtf8());
        QJsonArray jsonArr = jsonDoc.array();
        for (int i = 0; i < jsonArr.size(); i++) {
            QJsonObject infoJson = jsonArr[i].toObject();
            if (infoJson["displayname"] == "UOS AI") {
                EAiExec()->setCurrentAssistantId(infoJson["id"].toString());
                break;
            }
        }
    }

    // 每次发起请求时，复位错误码
    m_curErr = ERROR_TYPE_NONE;
    EAiExec()->cancelAiRequst(m_reqId);
    EAiExec()->clearAiRequest(this);

    m_replyCacheMutex.lock();
    m_replyCache.clear();
    m_replyCacheIdx = 0;
    m_replyCacheMutex.unlock();

    if (!m_reply.isEmpty()) {
        m_replyBak = m_reply;
    }
    m_reply.clear();
    m_replyTextEdit->clear();
    m_isReplyEditCursorEnd = true;

    m_replyTextEdit->setVisible(false);
    m_inProgressLabel->setVisible(true);
    m_inProgressMovie->start();

    // 每次发起请求时，隐藏下半部分控件
    this->setReplyPartVisible(false);
    this->enableReplyFunBt(false);
    this->asyncAdjustSize();

    // 规避偶现的AI请求卡住UI线程，至少也要让dialog先show出来
    QTimer::singleShot(100, this, [&] {
        qCInfo(logWordWizard) << "Sending AI request - model:" << m_modelInfo << "query:" << m_systemPrompt;
        m_reqId = EAiExec()->sendWordWizardRequest(m_modelId, m_systemPrompt, this, QString("onModelReply"));
    });
}

void AiQuickDialog::putAiReply(QString reply)
{
    while (m_replyCache.isEmpty() && reply.startsWith("\n")) {
        reply.remove(0, 1);
    }
    bool isStop = true;
    m_replyCacheMutex.lock();
    if (m_replyCacheIdx < m_replyCache.size()) {
        isStop = false;
    }
    m_replyCache += reply;
    m_replyCacheMutex.unlock();
    if (isStop) {
        this->smoothEachWord();
    }
}

void AiQuickDialog::smoothEachWord()
{
    if (!this->isVisible()) {
        return;
    }

    m_replyCacheMutex.lock();
    QString appendStr;
    if (m_replyCacheIdx < m_replyCache.size()) {
        QChar word = m_replyCache[m_replyCacheIdx];
        m_replyCacheIdx++;
        m_reply += word;
        appendStr += word;
        // 非中文的情况，按字母输出太慢了，尝试两个字母输出
        if (word.script() != QChar::Script_Han && m_replyCacheIdx < m_replyCache.size()) {
            word = m_replyCache[m_replyCacheIdx];
            m_replyCacheIdx++;
            m_reply += word;
            appendStr += word;
        }
    } else {
        m_replyCacheMutex.unlock();
        if (m_isReplyEnd) {
            this->setReplyPartVisible(true);
            this->enableReplyFunBt(true);
            this->asyncAdjustSize();
        }
        return;
    }
    m_replyCacheMutex.unlock();
    m_replyTextEdit->insertPlainText(appendStr);
    this->asyncAdjustSize();

    QTimer::singleShot(30, this, &AiQuickDialog::smoothEachWord);
}

void AiQuickDialog::continueDialog()
{
    Conversations conv;
    // question
    conv.question.chatType       = ChatAction::ChatTextPlain;
    conv.question.reqId          = m_reqId;
    conv.question.content        = m_systemPrompt;
    conv.question.displayContent = m_query;
    conv.question.displayHash    = QString(QCryptographicHash::hash(conv.question.displayContent.toUtf8(), QCryptographicHash::Md5).toHex());
    conv.question.llmIcon        = m_modelIconBak;
    conv.question.llmName        = m_modelNameBak;
    conv.question.llmId          = m_modelIdBak;
    conv.question.errCode        = 0;
    conv.question.errInfo        = "{\"info\":\"\", \"exec\":\"\"}";
    conv.question.isRetry        = false;

    QJsonObject rootJson;
    if (m_customFunctionList[m_queryType].defaultFunctionType != WordWizard::WIZARD_TYPE_TRANSLATE) {
        rootJson.insert("type", ExtentionType::WordSelection);
        rootJson.insert("label", QString("[%1]\n").arg(m_menuLabel));
    } else {
        rootJson.insert("type", ExtentionType::None);
    }
    rootJson.insert("prompt", m_systemPrompt);
    QJsonArray jsonArr;
    jsonArr.append(rootJson);
    conv.question.extention      = QString(QJsonDocument(jsonArr).toJson(QJsonDocument::Compact));

    // answer
    ChatChunk answer;

    QJsonArray displayContentArray;
    QJsonObject dialogContent;
    dialogContent.insert("content", m_reply);
    dialogContent.insert("chatType", ChatAction::ChatTextPlain);
    displayContentArray.append(dialogContent);

    answer.chatType       = ChatAction::ChatTextPlain;
    answer.reqId          = m_reqId;
    answer.content        = m_reply;
    answer.displayContent = QString(QJsonDocument(displayContentArray).toJson(QJsonDocument::Compact));
    answer.displayHash    = QString(QCryptographicHash::hash(answer.displayContent.toUtf8(), QCryptographicHash::Md5).toHex());
    answer.llmIcon        = m_modelIconBak;
    answer.llmName        = m_modelNameBak;
    answer.llmId          = m_modelIdBak;
    answer.errCode        = m_modelErrCode;
    answer.errInfo        = "{\"info\":\"\", \"exec\":\"\"}";
    answer.isRetry        = false;
    answer.extention      = "[]";
    answer.knowledgeSearchStatus = false;

    conv.answers.append(answer);

    if (EAiExec()->showChatWindow()) { // 等待前端渲染完成后再传，下一步会接收信号判断是否渲染完成
        m_isWebviewOk = false;
        QTimer::singleShot(2000, nullptr, [&, conv] {
            if (m_isWebviewOk) {
                qCInfo(logWordWizard) << "webview is ok!";
                if (m_customFunctionList[m_queryType].defaultFunctionType == WordWizard::WIZARD_TYPE_TRANSLATE) {
                   EAiExec()->wordWizardContinueChat(conv, AssistantType::AI_TRANSLATION);
                } else {
                   EAiExec()->wordWizardContinueChat(conv, AssistantType::AI_TEXT_PROCESSING);
                }
            }
            this->close();
        });
    } else {
        if (m_customFunctionList[m_queryType].defaultFunctionType == WordWizard::WIZARD_TYPE_TRANSLATE) {
           EAiExec()->wordWizardContinueChat(conv, AssistantType::AI_TRANSLATION);
        } else {
           EAiExec()->wordWizardContinueChat(conv, AssistantType::AI_TEXT_PROCESSING);
        }
        this->close();
    }
}

void AiQuickDialog::adjustReplyTextEditSize()
{
    if (!m_replyTextEdit->isVisible()) {
        return;
    }

    QTextBlockFormat blockFmt = m_replyTextEdit->textCursor().blockFormat();
    blockFmt.setLineHeight(122, QTextBlockFormat::LineHeightTypes::ProportionalHeight);
    m_replyTextEdit->textCursor().setBlockFormat(blockFmt);

    int height = static_cast<int>(m_replyTextEdit->document()->size().height()) + 4;
    int maxHeight = m_replyTextEditMax + 1 - m_queryTextEdit->height();
    m_replyTextEdit->setFixedHeight(height > maxHeight ? maxHeight : height);
    if (m_isReplyEditCursorEnd) {
        // 自动滚动到新内容处
        m_replyTextEdit->moveCursor(QTextCursor::End);
    }
}

void AiQuickDialog::syncLlmAccount()
{
    QString modelListJson = EAiExec()->queryUosAiLLMAccountList();
    m_modelId             = EAiExec()->uosAiLLMAccountId();
    int modelType = 0;
    qCInfo(logWordWizard) << "modelId:" << m_modelId << "modelList:" << modelListJson;
    // [{\"displayname\":\"ERNIE-Bot（百度千帆）-试用账号\",\"icon\":\"/tmp/uos-ai-assistant-WaznkA/baidu.svg\",\"id\":\"3x9Z1LXhJ1snR+1lAILd3hjyJmg4uWKTA15eMBo5PGOwlyfCpNgy0dsdl7GAM30s\",\"llmname\":\"ERNIE 3.5\",\"model\":20,\"type\":1},{\"displayname\":\"uos-baidu\",\"icon\":\"/tmp/uos-ai-assistant-WaznkA/baidu.svg\",\"id\":\"9a4348121e0d46c4a485af3f56817519\",\"llmname\":\"ERNIE 3.5\",\"model\":20,\"type\":0}]
    QJsonDocument jsonDoc = QJsonDocument::fromJson(modelListJson.toUtf8());
    QJsonArray jsonArr = jsonDoc.array();
    bool isFound = false;
    if (!jsonArr.isEmpty()) { // no model
        for (int i = 0; i < jsonArr.size(); i++) {
            QJsonObject infoJson = jsonArr[i].toObject();
            if (infoJson["id"].toString() == m_modelId) {
                isFound = true;
                m_modelName = infoJson["displayname"].toString();
                m_modelInfo = tr("Current model: ") + infoJson["displayname"].toString();
                m_modelIcon = infoJson["icon"].toString();
                modelType = infoJson["type"].toInt();
                break;
            }
        }

        // m_modelId没找到，使用第一个
        if (!isFound) {
            isFound = true;
            QJsonObject infoJson = jsonArr[0].toObject();
            m_modelId   = infoJson["id"].toString();
            m_modelName = infoJson["displayname"].toString();
            m_modelInfo = tr("Current model: ") + infoJson["displayname"].toString();
            m_modelIcon = infoJson["icon"].toString();
            modelType = infoJson["type"].toInt();

            //m_modelInfoLabel->setVisible(true);
            //m_modelIconLabel->setVisible(true);
        }
    }

    if (!isFound) {
        m_modelId.clear();
        m_modelInfo = tr("Currently no model");
    }

    this->asyncAdjustSize();
}

void AiQuickDialog::handleError()
{
    if (m_curErr == AiQuickDialog::ERROR_TYPE_NO_MODEL || m_curErr == AiQuickDialog::ERROR_TYPE_ACCOUNT_LIMIT || m_curErr == AiQuickDialog::ERROR_TYPE_ACCOUNT_EXPIRED || m_curErr == AiQuickDialog::ERROR_TYPE_ACCOUNT_INVALID) {
        this->enableReplyFunBt(false);
        if (!m_replyBak.isEmpty()) {
            m_cancelBt->setVisible(true);
        }
        const QColor &color = DPaletteHelper::instance()->palette(m_errorInfoLabel).color(DPalette::Normal, DPalette::Highlight);
        m_errorInfoLabel->setText(m_curErrInfo
                                  + QString("<a href=\"javascript:void(0)\" style=\"color:%1; text-decoration: none;\"> %2<img src=\"%3\"></a>")
                                        .arg(color.name())
                                        .arg(tr("Go to configure  "))
                                        .arg(GUtils::generateImage(m_errorInfoLabel, color, QSize(QFontMetrics(m_errorInfoLabel->font()).height() / 2, QFontMetrics(m_errorInfoLabel->font()).height() / 2), QStyle::PE_IndicatorArrowRight)));
        m_errorInfoLabel->setFixedHeight(m_errorInfoLabel->sizeHint().height());
        m_replyTextEdit->setVisible(false);
        m_errorInfoLabel->setVisible(true);

        this->asyncAdjustSize();
        return;
    }
}

// dialog内容要保持在屏幕范围内
void AiQuickDialog::adjustDialogPosition()
{
    int bottomLimit = 0;
    QList<QScreen *> screens = QGuiApplication::screens();
    for(QScreen *screen : screens) {
        if (screen->geometry().contains(this->geometry().x() + this->width() / 2, this->geometry().y())) {
            bottomLimit = screen->geometry().bottom();
            break;
        }
    }
    if (bottomLimit > 0 && this->geometry().bottom() > bottomLimit) {
        this->move(this->geometry().x(), this->geometry().y() - (this->geometry().bottom() - bottomLimit));
    }
}

void AiQuickDialog::asyncAdjustSize(int ms)
{
    QTimer::singleShot(ms, this, [&] {
        this->adjustReplyTextEditSize();
        this->adjustSize();
        this->adjustDialogPosition();
    });
}

void AiQuickDialog::enableReplyFunBt(bool isTrue)
{
    m_readBt->setEnabled(isTrue);
    m_replaceBt->setEnabled(isTrue);
    m_againBt->setEnabled(isTrue);
    m_copyBt->setEnabled(isTrue);
    m_continueBt->setEnabled(isTrue);

    dynamic_cast<WriterTextEdit *>(m_replyTextEdit)->setInProgress(!isTrue);
}

void AiQuickDialog::setReplyPartVisible(bool isTrue)
{
    qCDebug(logWordWizard) << "Setting reply part visibility:" << isTrue;

#ifdef COMPILE_ON_V25
    m_isWritable = m_isWritable || WordWizard::fcitxWritable();//尽可能保持m_isWritable为true
#endif
    m_readBt->setVisible(isReadAloudSupported() && isTrue && !m_translateAction->isVisible());
    m_replaceBt->setVisible(isTrue && m_isWritable);
    m_replySep1->setVisible(isTrue && m_isWritable);
    m_againBt->setVisible(isTrue);
    m_copyBt->setVisible(isTrue);

    m_vSpace->setVisible(isTrue);
    m_continueBt->setVisible(isTrue);

    m_attentionLabel->setVisible(isTrue);
    m_modelBt->setVisible(isTrue);
}

bool AiQuickDialog::isReadAloudSupported() const
{
    // 朗读功能，只能朗读中文和英语
    return kTargetLanguageIdx == LANGUAGE_TYPE_SIMPLIFIED_CHINESE
           // || kTargetLanguageIdx == LANGUAGE_TYPE_TRADITIONAL_CHINESE  // 繁体中文暂不支持朗读
            || kTargetLanguageIdx == LANGUAGE_TYPE_ENGLISH;
}

void AiQuickDialog::runOCRProcessByPath(int type, bool isCustom, const QString &imagePath)
{
    if (imagePath.isEmpty()) {
        qCWarning(logWordWizard) << "OCR process: Empty image path list";
        return;
    }

    if (false) {
        QString assisListJson = EAiExec()->queryAssistantList();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(assisListJson.toUtf8());
        QJsonArray jsonArr = jsonDoc.array();
        for (int i = 0; i < jsonArr.size(); i++) {
            QJsonObject infoJson = jsonArr[i].toObject();
            if (infoJson["displayname"] == "UOS AI") {
                EAiExec()->setCurrentAssistantId(infoJson["id"].toString());
                break;
            }
        }
    }

    // 每次发起请求时，复位错误码
    m_curErr = ERROR_TYPE_NONE;
    EAiExec()->cancelAiRequst(m_reqId);
    EAiExec()->clearAiRequest(this);

    m_replyCacheMutex.lock();
    m_replyCache.clear();
    m_replyCacheIdx = 0;
    m_replyCacheMutex.unlock();

    if (!m_reply.isEmpty()) {
        m_replyBak = m_reply;
    }
    m_reply.clear();
    m_replyTextEdit->clear();
    m_isReplyEditCursorEnd = true;

    m_querySep->setVisible(false);
    m_queryTextEdit->setVisible(false);
    m_inOCRLabel->setVisible(true);
    m_inOCRMovie->start();

    m_replyTextEdit->setVisible(false);
    m_inProgressLabel->setVisible(true);
    m_inProgressMovie->start();

    // 每次发起请求时，隐藏下半部分控件
    this->setReplyPartVisible(false);
    this->enableReplyFunBt(false);
    this->asyncAdjustSize();

    qCInfo(logWordWizard) << "Starting OCR process for images:" << imagePath;

    // 创建 QProcess 实例
    QProcess *ocrProcess = new QProcess(this);

    QStringList arguments;

    // 添加图片路径参数（用逗号分隔）
    arguments << "--images" << imagePath;

    // 添加语言参数
    arguments << "--language" << "zh-Hans_en";

    // 设置进程完成时的处理
    connect(ocrProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, ocrProcess, type, isCustom](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)

        qCInfo(logWordWizard) << "OCR process finished with exit code:" << exitCode;

        // 读取进程输出并收集结果
        QString allResults;
        QByteArray outputData = ocrProcess->readAllStandardOutput();
        QStringList lines = QString::fromUtf8(outputData).split('\n');

        for (const QString &line : lines) {
            if (line.trimmed().isEmpty()) continue;

            // 解析 JSON 输出
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &error);

            if (error.error != QJsonParseError::NoError) {
                qCWarning(logWordWizard) << "Failed to parse OCR output JSON:" << error.errorString();
                continue;
            }

            QJsonObject obj = doc.object();
            QString resultType = obj["type"].toString();

            if (resultType == "single_result") {
                // 单个图片结果
                QJsonObject data = obj["data"].toObject();
                bool success = data["success"].toBool();
                QString result = data["result"].toString();

                if (success) {
                    qCInfo(logWordWizard) << "OCR result :" << result;
                    if (!allResults.isEmpty()) {
                        allResults += "\n\n";
                    }
                    allResults += result;
                } else {
                    QString errorMsg = data["error"].toString();
                    qCWarning(logWordWizard) << "OCR failed :" << errorMsg;
                }
            }
        }

        qCInfo(logWordWizard) << "OCR process completed, returning accumulated results";

        if (allResults != "") {
            m_query = allResults;
            m_systemPrompt += m_query;
            m_query.replace("￼", " ");
        } else {
            m_query = tr("No text recognized.");
            m_replyTextEdit->setVisible(true);
            m_inProgressLabel->setVisible(false);
            m_inProgressMovie->stop();
        }

        m_querySep->setVisible(true);
        m_queryTextEdit->setVisible(true);
        m_inOCRLabel->setVisible(false);
        m_inOCRMovie->stop();
        m_queryTextEdit->setFullText(m_query);

        // 规避偶现的AI请求卡住UI线程，至少也要让dialog先show出来
        if (allResults != "") {
            QTimer::singleShot(100, this, [&] {
                qCInfo(logWordWizard) << "Sending AI request - model:" << m_modelInfo << "query:" << m_systemPrompt;
                m_reqId = EAiExec()->sendWordWizardRequest(m_modelId, m_systemPrompt, this, QString("onModelReply"));
            });
        }
        // 清理进程对象防止内存泄漏
        ocrProcess->deleteLater();
    });

    // 设置进程错误处理
    connect(ocrProcess, &QProcess::errorOccurred,
            [this, ocrProcess](QProcess::ProcessError error) {
        qCWarning(logWordWizard) << "OCR process error occurred:" << error;

        // 出错时恢复UI状态
        m_querySep->setVisible(true);
        m_queryTextEdit->setVisible(true);
        m_inOCRLabel->setVisible(false);
        m_inOCRMovie->stop();

        m_query = tr("No text recognized.");
        m_replyTextEdit->setVisible(true);
        m_inProgressLabel->setVisible(false);
        m_inProgressMovie->stop();
        m_queryTextEdit->setFullText(m_query);

        // 清理进程对象防止内存泄漏
        ocrProcess->deleteLater();
    });

    // 设置标准错误输出读取处理
    connect(ocrProcess, &QProcess::readyReadStandardError,
            [this, ocrProcess]() {
        QByteArray data = ocrProcess->readAllStandardError();
        QString errorOutput = QString::fromUtf8(data).trimmed();
        if (!errorOutput.isEmpty()) {
            // 过滤掉一些常见的非关键警告信息
            if (!errorOutput.contains("QML") &&
                    !errorOutput.contains("libpng warning") &&
                    !errorOutput.contains("deprecated")) {
                qCWarning(logWordWizard) << "OCR process error output:" << errorOutput;
            } else {
                qCDebug(logWordWizard) << "OCR process debug output:" << errorOutput;
            }
        }
    });

    // 设置 OCR 进程和参数（调试的时候可以手动切换到build路径）
    QString program = "/usr/lib/uos-ai-assistant/uos-ai-ocr-process";

    qCInfo(logWordWizard) << "Starting OCR process with program:" << program
                     << "arguments:" << arguments;

    // 启动进程（非阻塞方式）
    ocrProcess->start(program, arguments);

    // 检查进程是否启动成功（非阻塞方式）
    if (!ocrProcess->waitForStarted(50000)) { // 5 秒超时
        qCWarning(logWordWizard) << "Failed to start OCR process:" << ocrProcess->errorString();

        // 出错时恢复UI状态
        m_querySep->setVisible(true);
        m_queryTextEdit->setVisible(true);
        m_inOCRLabel->setVisible(false);
        m_inOCRMovie->stop();

        m_query = tr("No text recognized.");
        m_replyTextEdit->setVisible(true);
        m_inProgressLabel->setVisible(false);
        m_inProgressMovie->stop();
        m_queryTextEdit->setFullText(m_query);

        ocrProcess->deleteLater();
        return;
    }

    qCInfo(logWordWizard) << "OCR process started successfully";
}

void AiQuickDialog::updateKnowledgeActionEnabled()
{
    if (!m_knowledgeAction)
        return;

    bool enabled = m_query.length() >= 10;
    QString tooltip = enabled ? "" : tr("The added content must be more than 10 words");
    m_knowledgeAction->setEnabled(enabled);
    m_knowledgeAction->setToolTip(tooltip);
}
