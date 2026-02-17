#include "aiwriterdialog.h"
#include "aiquickdialog.h"
#include "private/transbutton.h"
#include "private/filltextbutton.h"
#include "private/hinttextedit.h"
#include "private/circlebutton.h"
#include "private/writertextedit.h"
#include "../wordwizard.h"
#include <gui/chat/private/eaiexecutor.h>
#include <gui/chat/chatwindow.h>
#include <llm/common/networkdefs.h>
#include <wrapper/serverwrapper.h>
#include <dbus/fcitxinputserver.h>
#include <gui/gutils.h>
#include <utils/dconfigmanager.h>
#include "private/welcomedialog.h"
#include "utils/esystemcontext.h"
#include <report/writerpoint.h>
#include <report/writerfunctionpoint.h>
#include <report/eventlogutil.h>

#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DDialog>
#include <DPaletteHelper>

#include "ddedockobject.h"

#include <QScreen>
#include <QTimer>
#include <QClipboard>
#include <QTextBlockFormat>
#include <QScrollBar>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logWordWizard)

#ifdef COMPILE_ON_V25
#include <ddeshellwayland.h>
#endif

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
DCORE_USE_NAMESPACE
using namespace uos_ai;

static constexpr char WARNING_ICON[] = ":/assets/images/warning.svg";

AiWriterDialog &AiWriterDialog::instance() {
    static AiWriterDialog instance;
    return instance;
}

AiWriterDialog::AiWriterDialog(DWidget *parent) : DAbstractDialog(parent)
{
    // 避免Dialog关闭后，带起主窗口（~Qt::Dialog）
    this->setWindowFlags((this->windowFlags() | Qt::WindowStaysOnTopHint) & ~Qt::Dialog);
    initUi();
    initConnect();

    // 第一次使用填充功能，提醒用户重启输入法服务
    m_isFirstFill = DConfigManager::instance()->value(AIQUICK_GROUP, AIQUICK_ISFIRSTFILL, true).toBool();

}

AiWriterDialog::~AiWriterDialog() {
}

void AiWriterDialog::initUi() {
    this->setFixedWidth(650);
    this->setMaximumHeight(600);
#ifdef COMPILE_ON_V25
    this->move(100, 100);
#endif

    //标题栏
    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);

    m_logoBt = new TransButton(this);
    m_logoBt->setIcon(QIcon::fromTheme("uos-ai-assistant"));
    m_logoBt->setIconSize(QSize(25, 25));
    dynamic_cast<TransButton *>(m_logoBt)->setIconNoPressColor();

    DLabel *titleLable = new DLabel(tr("AI Writer"), this);
    DFontSizeManager::instance()->bind(titleLable, DFontSizeManager::T7, QFont::Normal);

    QHBoxLayout *allTitleLayout = new QHBoxLayout();
    allTitleLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    allTitleLayout->addSpacing(9);
    allTitleLayout->addWidget(m_logoBt);
    allTitleLayout->addWidget(titleLable);
    allTitleLayout->addWidget(titleBar);

    m_queryEdit = new HintTextEdit(this);
    m_queryEdit->setPlaceholderText(tr("Enter what you want to create (press Enter to generate/Esc to exit)"));
    m_queryEdit->setFrameShape(QFrame::NoFrame);
    m_queryEdit->setLineWrapMode(QTextEdit::WidgetWidth);
    m_queryEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    DPalette queryPalette = m_queryEdit->palette();
    queryPalette.setColor(DPalette::Base, QColor(255, 255, 255, 0));
    m_queryEdit->setPalette(queryPalette);
    m_queryEditMin = static_cast<int>(m_queryEdit->document()->size().height()) + 4;
    m_queryEdit->setFixedHeight(m_queryEditMin);
    m_queryEditMax = this->maximumHeight();
    m_queryEdit->installEventFilter(this);
    DFontSizeManager::instance()->bind(m_queryEdit, DFontSizeManager::T7, QFont::Normal);

    m_sendBt = new CircleButton(this);
    m_sendBt->setIcon(QIcon::fromTheme("uos-ai-assistant_aisend"));
    m_sendBt->setIconSize(QSize(14, 14));
    m_sendBt->setFixedSize(QSize(24, 24));

    m_querySpace = new DWidget(this);
    // 实际大小应该是15-4=11，实测后选择用8
    m_querySpace->setFixedWidth(8);

    QHBoxLayout *queryLayout = new QHBoxLayout();
    queryLayout->setContentsMargins(10, 0, 2, 0);
    queryLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    queryLayout->addWidget(m_queryEdit);
    queryLayout->addWidget(m_sendBt);
    queryLayout->addWidget(m_querySpace);

    m_sendBt1 = new CircleButton(this);
    m_sendBt1->setIcon(QIcon::fromTheme("uos-ai-assistant_aisend"));
    m_sendBt1->setIconSize(QSize(14, 14));
    m_sendBt1->setFixedSize(QSize(24, 24));
    m_sendBt1->setVisible(false);

    QHBoxLayout *sendLayout = new QHBoxLayout();
    sendLayout->setContentsMargins(15, 0, 15, 0);
    sendLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sendLayout->addStretch();
    sendLayout->addWidget(m_sendBt1);

    m_sendVSpace = new DWidget(this);
    m_sendVSpace->setFixedHeight(0);
    m_sendVSpace->setVisible(false);

    m_querySep = new DWidget(this);
    m_querySep->setFixedSize(2, 1);
    DPalette querySepPalette = m_querySep->palette();
    querySepPalette.setColor(DPalette::Window, QColor(0, 0, 0, 76));
    m_querySep->setPalette(querySepPalette);
    m_querySep->setAutoFillBackground(true);
    m_querySep->setVisible(false);

    m_queryTextEdit = new QueryTextEdit(this);
    m_queryTextEdit->setContentsMargins(0, 0, 0, 0);
    m_queryTextEdit->setReadOnly(true);
    m_queryTextEdit->setFrameShape(QFrame::NoFrame);
    m_queryTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_queryTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_queryTextEdit->setWordWrapMode(QTextOption::WrapMode::WrapAnywhere);
    DPalette queryTextPalette = m_queryTextEdit->palette();
    queryTextPalette.setColor(DPalette::Base, QColor(255, 255, 255, 0));
    queryTextPalette.setColor(DPalette::Text, QColor(0, 0, 0, int(255 * 0.5)));
    m_queryTextEdit->setPalette(queryTextPalette);
    m_queryTextEdit->setFixedHeight(1);
    m_queryTextEdit->setFixedWidth(this->width() - 25 - m_querySep->width() - 4);
    DFontSizeManager::instance()->bind(m_queryTextEdit, DFontSizeManager::T7, QFont::Normal);
    m_queryTextEdit->setVisible(false);

    QHBoxLayout *queryTextLayout = new QHBoxLayout();
    queryTextLayout->setContentsMargins(15, 0, 10, 0);
    queryTextLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    queryTextLayout->addWidget(m_querySep);
    queryTextLayout->addWidget(m_queryTextEdit);

    m_queryHSep = new DWidget(this);
    m_queryHSep->setFixedSize(this->width() - 30, 1);
    DPalette queryHSepPalette = m_queryHSep->palette();
    queryHSepPalette.setColor(DPalette::Window, QColor(0, 0, 0, 25));
    m_queryHSep->setPalette(queryHSepPalette);
    m_queryHSep->setAutoFillBackground(true);
    m_queryHSep->setVisible(false);

    m_queryHSepVSpace = new DWidget(this);
    m_queryHSepVSpace->setFixedHeight(5);
    m_queryHSepVSpace->setVisible(false);

    QVBoxLayout *queryHSepLayout = new QVBoxLayout();
    queryHSepLayout->setContentsMargins(15, 0, 15, 0);
    queryHSepLayout->addWidget(m_queryHSep);
    queryHSepLayout->addWidget(m_queryHSepVSpace);

    int articleBtWidth = 82;
    m_articleBt = new FillTextButton(tr("Article"), this);
    m_articleBt->setFixedWidth(articleBtWidth);
    m_articleBt->setIcon(QIcon::fromTheme("uos-ai-assistant_article"));
    m_articleBt->setIconSize(QSize(22, 22));
    m_outlineBt = new FillTextButton(tr("Outline"), this);
    m_outlineBt->setFixedWidth(articleBtWidth);
    m_outlineBt->setIcon(QIcon::fromTheme("uos-ai-assistant_outline"));
    m_outlineBt->setIconSize(QSize(22, 22));
    m_notifyBt = new FillTextButton(tr("Notification"), this);
    m_notifyBt->setFixedWidth(articleBtWidth);
    m_notifyBt->setIcon(QIcon::fromTheme("uos-ai-assistant_notify"));
    m_notifyBt->setIconSize(QSize(22, 22));
    m_reportBt = new FillTextButton(tr("Research Report"), this);
    m_reportBt->setFixedWidth(articleBtWidth);
    m_reportBt->setIcon(QIcon::fromTheme("uos-ai-assistant_report"));
    m_reportBt->setIconSize(QSize(22, 22));
    m_speechBt = new FillTextButton(tr("Speeches"), this);
    m_speechBt->setFixedWidth(articleBtWidth);
    m_speechBt->setIcon(QIcon::fromTheme("uos-ai-assistant_speech"));
    m_speechBt->setIconSize(QSize(22, 22));
    m_summaryBt = new FillTextButton(tr("Work Summary"), this);
    m_summaryBt->setFixedWidth(articleBtWidth);
    m_summaryBt->setIcon(QIcon::fromTheme("uos-ai-assistant_worksummary"));
    m_summaryBt->setIconSize(QSize(22, 22));
    m_publicBt = new FillTextButton(tr("Tweets"), this);
    m_publicBt->setFixedWidth(articleBtWidth);
    m_publicBt->setIcon(QIcon::fromTheme("uos-ai-assistant_official_accounts"));
    m_publicBt->setIconSize(QSize(22, 22));
    DFontSizeManager::instance()->bind(m_articleBt, DFontSizeManager::T10, QFont::Normal);
    DFontSizeManager::instance()->bind(m_outlineBt, DFontSizeManager::T10, QFont::Normal);
    DFontSizeManager::instance()->bind(m_notifyBt, DFontSizeManager::T10, QFont::Normal);
    DFontSizeManager::instance()->bind(m_reportBt, DFontSizeManager::T10, QFont::Normal);
    DFontSizeManager::instance()->bind(m_speechBt, DFontSizeManager::T10, QFont::Normal);
    DFontSizeManager::instance()->bind(m_summaryBt, DFontSizeManager::T10, QFont::Normal);
    DFontSizeManager::instance()->bind(m_publicBt, DFontSizeManager::T10, QFont::Normal);

    QHBoxLayout *articalLayout = new QHBoxLayout();
    articalLayout->setContentsMargins(15, 0, 15, 0);
    articalLayout->setAlignment(Qt::AlignCenter);
    articalLayout->addWidget(m_articleBt);
    articalLayout->addStretch();
    articalLayout->addWidget(m_outlineBt);
    articalLayout->addStretch();
    articalLayout->addWidget(m_notifyBt);
    articalLayout->addStretch();
    articalLayout->addWidget(m_reportBt);
    articalLayout->addStretch();
    articalLayout->addWidget(m_speechBt);
    articalLayout->addStretch();
    articalLayout->addWidget(m_summaryBt);
    articalLayout->addStretch();
    articalLayout->addWidget(m_publicBt);

    m_articleVSpace = new DWidget(this);
    m_articleVSpace->setFixedHeight(0);
    m_articleVSpace1 = new DWidget(this);
    m_articleVSpace1->setFixedHeight(5);

    m_replyEdit = new WriterTextEdit(this);
    m_replyEdit->setFrameShape(QFrame::NoFrame);
    m_replyEdit->document()->setDocumentMargin(10);
    m_replyEdit->setReadOnly(true);
    //m_replyEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    DPalette replyPalette = m_replyEdit->palette();
    replyPalette.setColor(DPalette::Base, QColor(255, 255, 255, 0));
    m_replyEdit->setPalette(replyPalette);
    m_replyEdit->installEventFilter(this);
    m_replyEdit->setVisible(false);
    DFontSizeManager::instance()->bind(m_replyEdit, DFontSizeManager::T7, QFont::Normal);

    m_highlighter = new MarkdownHighlighter(m_replyEdit->document());

    m_errorInfoLabel = new DLabel(this);
    m_errorInfoLabel->setWordWrap(true);
    m_errorInfoLabel->setFixedWidth(this->maximumWidth() - 30);
    m_errorInfoLabel->setContextMenuPolicy(Qt::NoContextMenu);
    m_errorInfoLabel->setVisible(false);

    m_cancelBt = new DPushButton(tr("Cancel"), this);
    m_cancelBt->setFixedHeight(20);
    m_cancelBt->setFixedWidth(m_cancelBt->sizeHint().width());
    DPalette cancelPalette = m_cancelBt->palette();
    cancelPalette.setColor(DPalette::ButtonText, QColor(0, 0, 0, 178));
    m_cancelBt->setPalette(cancelPalette);
    DFontSizeManager::instance()->bind(m_cancelBt, DFontSizeManager::T8, QFont::Normal);
    m_cancelBt->setVisible(false);

    m_inProgressMovie = new QMovie(":assets/images/light/light-loading.gif");
    m_inProgressLabel = new DLabel(this);
    m_inProgressLabel->setMovie(m_inProgressMovie);
    // original size 800x83
    int inProgressWidth = this->width() - 30;
    float inProgressRatio = 800.0F / inProgressWidth;
    int inProgressHeight = 38;//static_cast<int>(83.0F / inProgressRatio);
    m_inProgressLabel->setFixedSize(inProgressWidth, inProgressHeight);
    m_inProgressLabel->setScaledContents(true);
    m_inProgressLabel->setVisible(false);

    m_progressVSpace = new DWidget(this);
    m_progressVSpace->setFixedHeight(1);
    m_progressVSpace->setVisible(false);

    QVBoxLayout *replyLayout = new QVBoxLayout();
    replyLayout->setContentsMargins(5, 0, 2, 0);
    replyLayout->setAlignment(Qt::AlignLeft);
    replyLayout->addWidget(m_replyEdit);

    QVBoxLayout *replyLayout1 = new QVBoxLayout();
    replyLayout1->setContentsMargins(15, 0, 15, 0);
    replyLayout1->setAlignment(Qt::AlignLeft);
    replyLayout1->addWidget(m_inProgressLabel);
    replyLayout1->addWidget(m_progressVSpace);
    replyLayout1->addWidget(m_errorInfoLabel);
    replyLayout1->addWidget(m_cancelBt);

    // replace
    m_replaceBt = new TransButton(tr("Paste to Text"), this);
    m_replaceBt->setIcon(QIcon::fromTheme("uos-ai-assistant_replace"));
    m_replaceBt->setIconSize(QSize(16, 16));
    DPalette replacePalette = m_replaceBt->palette();
    replacePalette.setColor(DPalette::ButtonText, QColor(0, 0, 0, 178));
    m_replaceBt->setPalette(replacePalette);
    DFontSizeManager::instance()->bind(m_replaceBt, DFontSizeManager::T8, QFont::Normal);
    m_replaceBt->setVisible(false);

    m_replySep = new DWidget(this);
    m_replySep->setFixedSize(1, 15);
    DPalette replyPalette1 = m_replySep->palette();
    replyPalette1.setColor(DPalette::Window, QColor(0, 0, 0, 25));
    m_replySep->setPalette(replyPalette1);
    m_replySep->setAutoFillBackground(true);
    m_replySep->setVisible(false);

    // again
    m_againBt = new TransButton(tr("Regenerate"), this);
    m_againBt->setIcon(QIcon::fromTheme("uos-ai-assistant_again"));
    m_againBt->setIconSize(QSize(16, 16));
    m_againBt->setPalette(replacePalette);
    DFontSizeManager::instance()->bind(m_againBt, DFontSizeManager::T8, QFont::Normal);
    m_againBt->setVisible(false);

    // copy
    m_copyBt = new TransButton(tr("Copy"), this);
    m_copyBt->setIcon(QIcon::fromTheme("uos-ai-assistant_copy"));
    m_copyBt->setIconSize(QSize(16, 16));
    m_copyBt->setPalette(replacePalette);
    DFontSizeManager::instance()->bind(m_copyBt, DFontSizeManager::T8, QFont::Normal);
    m_copyBt->setVisible(false);

    QHBoxLayout *replyFunLayout = new QHBoxLayout();
    replyFunLayout->setContentsMargins(10, 0, 10, 0);
    replyFunLayout->setAlignment(Qt::AlignLeft);
    replyFunLayout->addWidget(m_replaceBt);
    //replyFunLayout->addSpacing(5);
    replyFunLayout->addWidget(m_replySep);
    //replyFunLayout->addSpacing(5);
    replyFunLayout->addWidget(m_againBt);
    replyFunLayout->addWidget(m_copyBt);

    m_attentionLabel = new DLabel(this);
    m_attentionLabel->setText(tr("The content generated by AI is for reference only, please pay attention to the accuracy of the information."));
    QFont attentionFont = m_attentionLabel->font();
    attentionFont.setPixelSize(10);
    attentionFont.setWeight(QFont::Normal);
    m_attentionLabel->setFont(attentionFont);
    DPalette attentionPalette = m_attentionLabel->palette();
    attentionPalette.setColor(DPalette::WindowText, QColor(0, 0, 0, 102));
    m_attentionLabel->setPalette(attentionPalette);
    m_attentionLabel->setVisible(false);

    m_modelBt = new TransButton(this);
    m_modelBt->setIcon(QIcon::fromTheme("uos-ai-assistant_modelbt"));
    m_modelBt->setIconSize(QSize(16, 16));
    dynamic_cast<TransButton *>(m_modelBt)->setIconNoPressColor(true);
    m_modelBt->installEventFilter(this);
    m_attentionLabel->setFixedWidth(this->width() - 25 - m_modelBt->sizeHint().width() - 4);
    m_modelBt->setVisible(false);

    QHBoxLayout *attentionLayout = new QHBoxLayout();
    attentionLayout->setContentsMargins(15, 0, 10, 0);
    attentionLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    attentionLayout->addWidget(m_attentionLabel);
    attentionLayout->addWidget(m_modelBt);

    m_vSpace = new DWidget(this);
    m_vSpace->setFixedHeight(3);
    m_vSpace->setVisible(false);

    QVBoxLayout *allLayout = new QVBoxLayout(this);
    allLayout->setContentsMargins(0, 0, 0, 0);
    allLayout->setAlignment(Qt::AlignTop);
    allLayout->addLayout(allTitleLayout);
    allLayout->addLayout(queryLayout);
    allLayout->addLayout(sendLayout);
    allLayout->addWidget(m_sendVSpace);
    allLayout->addLayout(queryTextLayout);
    allLayout->addLayout(queryHSepLayout);
    allLayout->addWidget(m_articleVSpace1);
    allLayout->addLayout(articalLayout);
    allLayout->addWidget(m_articleVSpace);
    allLayout->addLayout(replyLayout);
    allLayout->addLayout(replyLayout1);
    allLayout->addWidget(m_vSpace);
    allLayout->addLayout(replyFunLayout);
    allLayout->addSpacing(3);
    allLayout->addLayout(attentionLayout);
    allLayout->addSpacing(5);
    allLayout->addStretch();
    this->setLayout(allLayout);

    m_isNeedResetUi = true;

#ifdef COMPILE_ON_V25
    if (ESystemContext::isTreeland()) {
        this->createWinId();
        DDEShellWayland::get(windowHandle())->setRole(QtWayland::treeland_dde_shell_surface_v1::role_overlay);
    }
#endif
}

void AiWriterDialog::resetUi() {
    m_curErr = AiQuickDialog::ERROR_TYPE_NONE;
    m_queryEdit->setFirstQueryFlag(true);
    m_query.clear();
    m_queryEdit->setText(m_query);
    m_queryEdit->setPlaceholderText(tr("Enter what you want to create (press Enter to generate/Esc to exit)"));
    m_queryEditMax = this->maximumHeight();
    m_queryEdit->setLineWrapMode(QTextEdit::WidgetWidth);

    m_querySep->setVisible(false);
    m_queryTextEdit->setVisible(false);
    m_queryHSep->setVisible(false);
    m_queryHSepVSpace->setVisible(false);

    m_reply.clear();
    m_replyBak.clear();
    m_replyCacheMutex.lock();
    m_replyCache.clear();
    m_replyCacheIdx = 0;
    m_replyCacheMutex.unlock();

    this->setArticleBtPartVisible(true);
    m_curBt = nullptr;
    m_inProgressLabel->setVisible(false);
    m_inProgressMovie->stop();
    m_progressVSpace->setVisible(false);
    m_errorInfoLabel->setVisible(false);
    m_cancelBt->setVisible(false);

    m_replyEdit->clear();
    m_replyEdit->setFixedHeight(0);
    m_replyEdit->setVisible(false);
    this->setReplyPartVisible(false);

    this->asyncAdjustSize();
}

void AiWriterDialog::initConnect() {
    connect(m_logoBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_sendBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_sendBt1, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_articleBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_outlineBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_notifyBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_reportBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_speechBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_summaryBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_publicBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_cancelBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_replaceBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_againBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);
    connect(m_copyBt, &DPushButton::clicked, this, &AiWriterDialog::onBtClicked);

    connect(m_queryEdit, &DTextEdit::textChanged, this, &AiWriterDialog::onTextChanged);

    connect(m_errorInfoLabel, &DLabel::linkActivated, this, &AiWriterDialog::onOpenConfigDialog);

    connect(m_replyEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [&] (int value) {
        if (!m_isReplyEditCursorEnd && value == m_replyEdit->verticalScrollBar()->maximum()) {
            m_isReplyEditCursorEnd = true;
        } else if (m_isReplyEditCursorEnd && m_replyEdit->verticalScrollBar()->maximum() - value > 50) {
            m_isReplyEditCursorEnd = false;
        }
    });

    // theme
    this->onUpdateSystemTheme(DGuiApplicationHelper::instance()->themeType());
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &AiWriterDialog::onUpdateSystemTheme);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, &AiWriterDialog::onFontChanged);
}

void AiWriterDialog::onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &themeType)
{
    this->onQueryTextChanged(true);
    this->update();

    if (themeType == DGuiApplicationHelper::LightType) {
        DPalette queryPalette = m_queryEdit->palette();
        queryPalette.setColor(DPalette::Base, QColor(255, 255, 255, 0));
        m_queryEdit->setPalette(queryPalette);

        DPalette replyPalette = m_replyEdit->palette();
        replyPalette.setColor(DPalette::Base, QColor(255, 255, 255, 0));
        m_replyEdit->setPalette(replyPalette);

        DPalette querySepPalette = m_querySep->palette();
        querySepPalette.setColor(DPalette::Window, QColor(0, 0, 0, 76));
        m_querySep->setPalette(querySepPalette);

        DPalette queryHSepPalette = m_queryHSep->palette();
        queryHSepPalette.setColor(DPalette::Window, QColor(0, 0, 0, 25));
        m_queryHSep->setPalette(queryHSepPalette);

        DPalette queryTextPalette = m_queryTextEdit->palette();
        queryTextPalette.setColor(DPalette::Base, QColor(255, 255, 255, 0));
        queryTextPalette.setColor(DPalette::Text, QColor(0, 0, 0, int(255 * 0.5)));
        m_queryTextEdit->setPalette(queryTextPalette);

        DPalette copyPalette = m_copyBt->palette();
        copyPalette.setColor(DPalette::ButtonText, QColor(0, 0, 0, 153));
        if (!this->isActiveWindow()) {
            DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
            copyPalette.setColor(DPalette::ButtonText, parentPb.color(DPalette::Inactive, DPalette::ButtonText));
        }
        m_copyBt->setPalette(copyPalette);
        m_cancelBt->setPalette(copyPalette);
        m_replaceBt->setPalette(copyPalette);
        m_againBt->setPalette(copyPalette);

        DPalette replyPalette1 = m_replySep->palette();
        replyPalette1.setColor(DPalette::Window, QColor(0, 0, 0, 25));
        m_replySep->setPalette(replyPalette1);

        DPalette attentionPalette = m_attentionLabel->palette();
        attentionPalette.setColor(DPalette::WindowText, QColor(0, 0, 0, 102));
        if (!this->isActiveWindow()) {
            DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
            attentionPalette.setColor(DPalette::WindowText, parentPb.color(DPalette::Inactive, DPalette::WindowText));
        }
        m_attentionLabel->setPalette(attentionPalette);

        m_inProgressMovie->setFileName(":assets/images/light/light-loading.gif");
        if (m_inProgressMovie->state() == QMovie::Running) {
            m_inProgressMovie->stop();
            m_inProgressMovie->start();
        }
    } else {
        DPalette queryPalette = m_queryEdit->palette();
        queryPalette.setColor(DPalette::Base, QColor(0, 0, 0, 0));
        m_queryEdit->setPalette(queryPalette);

        DPalette replyPalette = m_replyEdit->palette();
        replyPalette.setColor(DPalette::Base, QColor(0, 0, 0, 0));
        m_replyEdit->setPalette(replyPalette);

        DPalette querySepPalette = m_querySep->palette();
        querySepPalette.setColor(DPalette::Window, QColor(255, 255, 255, 76));
        m_querySep->setPalette(querySepPalette);

        DPalette queryHSepPalette = m_queryHSep->palette();
        queryHSepPalette.setColor(DPalette::Window, QColor(255, 255, 255, 25));
        m_queryHSep->setPalette(queryHSepPalette);

        DPalette queryTextPalette = m_queryTextEdit->palette();
        queryTextPalette.setColor(DPalette::Base, QColor(255, 255, 255, 0));
        queryTextPalette.setColor(DPalette::Text, QColor(255, 255, 255, int(255 * 0.5)));
        m_queryTextEdit->setPalette(queryTextPalette);

        DPalette copyPalette = m_copyBt->palette();
        copyPalette.setColor(DPalette::ButtonText, QColor(255, 255, 255, 153));
        if (!this->isActiveWindow()) {
            DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
            copyPalette.setColor(DPalette::ButtonText, parentPb.color(DPalette::Inactive, DPalette::ButtonText));
        }
        m_copyBt->setPalette(copyPalette);
        m_cancelBt->setPalette(copyPalette);
        m_replaceBt->setPalette(copyPalette);
        m_againBt->setPalette(copyPalette);

        DPalette replyPalette1 = m_replySep->palette();
        replyPalette1.setColor(DPalette::Window, QColor(255, 255, 255, 25));
        m_replySep->setPalette(replyPalette1);

        DPalette attentionPalette = m_attentionLabel->palette();
        attentionPalette.setColor(DPalette::WindowText, QColor(255, 255, 255, 102));
        if (!this->isActiveWindow()) {
            DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
            attentionPalette.setColor(DPalette::WindowText, parentPb.color(DPalette::Inactive, DPalette::WindowText));
        }
        m_attentionLabel->setPalette(attentionPalette);

        m_inProgressMovie->setFileName(":assets/images/dark/dark-loading.gif");
        if (m_inProgressMovie->state() == QMovie::Running) {
            m_inProgressMovie->stop();
            m_inProgressMovie->start();
        }
    }
    this->update();
}

void AiWriterDialog::showDialog() {
    qCDebug(logWordWizard) << "Showing writer dialog";
    if (!m_isInitOk) {
        // 需要延后connect EAiExec()，不然会造成EAiExec()的初始化问题
        connect(EAiExec(), &EAiExecutor::uosAiLlmAccountLstChanged, this, &AiWriterDialog::onUosAiLlmAccountLstChanged);
        connect(EAiExec(), &EAiExecutor::llmAccountLstChanged, this, &AiWriterDialog::onLlmAccountLstChanged);
        connect(EAiExec(), &EAiExecutor::netStateChanged, this, &AiWriterDialog::onNetworkStateChanged);
        m_isInitOk = true;
    }

    if (!this->isVisible()) {
        if (m_isNeedResetUi) {
            this->adjustSize();
            this->adjustDialogInitPosition();
        }
        this->show();

        // tid:1001600003 event:writer
        ReportIns()->writeEvent(report::WriterPoint().assemblingData());
    }
    this->activateWindow();
}

void AiWriterDialog::closeDialog() {
    qCDebug(logWordWizard) << "Closing writer dialog";
}

void AiWriterDialog::showEvent(QShowEvent *event) {
    qCDebug(logWordWizard) << "Showing, need reset UI:" << m_isNeedResetUi;
    DAbstractDialog::showEvent(event);
    m_isShow = true;
}

void AiWriterDialog::reject() {
    //qInfo() << QString("reject() trigger this->close()");
    m_isNeedResetUi = true;
    this->close();

    return DAbstractDialog::reject();
}

void AiWriterDialog::closeEvent(QCloseEvent *event) {
    qCDebug(logWordWizard) << "Closing writer dialog";
    EAiExec()->cancelAiRequst(m_reqId);
    EAiExec()->clearAiRequest(this);
    m_isShow = false;
    m_isNeedResetUi = true;
    this->resetUi();

    return DAbstractDialog::closeEvent(event);
}

void AiWriterDialog::changeEvent(QEvent *event) {
    //qInfo() << QString("changeEvent()");
    if (this->isVisible()) {
        this->onUpdateSystemTheme(DGuiApplicationHelper::instance()->themeType());
    }

    return DAbstractDialog::changeEvent(event);
}

void AiWriterDialog::moveEvent(QMoveEvent *event) {
    if (m_initPos != QPoint(0, 0) && event->oldPos() == m_initPos && event->pos() == QPoint(0, 0)) {
        qDebug() << QString("need forbid pos reset to 0");
        QTimer::singleShot(10, this, [&] {
            this->move(m_initPos);
        });
    }
    DAbstractDialog::moveEvent(event);
}

bool AiWriterDialog::eventFilter(QObject *watched, QEvent *event) {
    //qInfo() << event->type();
    // 是否启用此代码取决于基类
    if (false) {
        // 响应ESC按键
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                m_isNeedResetUi = true;
                this->close();
                return true;
            }
        } else if (event->type() == QEvent::KeyRelease) {
            QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                return true;
            }
        }
    }

    if (watched == m_queryEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(event);
            if (keyEvent->modifiers() == Qt::ControlModifier && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {
                if (!m_queryEdit->isFirstQuery()) {
                    return true;
                }

                m_queryEdit->insertPlainText("\n");
                return true;
            } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                if (m_sendBt->isEnabled()) {
                    m_sendBt->click();
                }
                return true;
            }
        } else if (event->type() == QEvent::KeyRelease) {
            QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(event);
            if (keyEvent->modifiers() == Qt::ControlModifier && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {
                return true;
            } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                return true;
            }
        }
    }

    return DAbstractDialog::eventFilter(watched, event);
}

void AiWriterDialog::onBtClicked() {
    if (sender() == m_curBt) { // 重复点同一个类别型button，不响应
        return;
    }

    if (sender() == m_logoBt) {
        WordWizard::onIconBtnClicked();
        return;
    }

    if (sender() == m_sendBt || sender() == m_sendBt1) {
        if (!WelcomeDialog::isAgreed()) {
            WelcomeDialog::instance(false)->exec();
            return;
        }
        this->sendAiRequst();
        return;
    }

    // tid:1001600004 event:writer_function
    if (sender() == m_articleBt) {
        qCDebug(logWordWizard) << "click article button";
        QString topic;
        if (m_curBt) {
            m_curBt->clearPressColor();
        } else {
            topic = m_queryEdit->toPlainText();
        }
        m_curBt = dynamic_cast<FillTextButton *>(sender());
        m_queryEdit->setText(QString(tr("Help me write an essay on the topic of [%1] with [clear structure and rich content].")).arg(topic.isEmpty() ? tr("Artificial Intelligence") : topic));
        m_queryEdit->moveCursor(QTextCursor::End);
        m_queryEdit->setFocus();
        ReportIns()->writeEvent(report::WriterFunctionPoint("essay").assemblingData());
        return;
    }

    if (sender() == m_outlineBt) {
        qCDebug(logWordWizard) << "click outline button";
        QString topic;
        if (m_curBt) {
            m_curBt->clearPressColor();
        } else {
            topic = m_queryEdit->toPlainText();
        }
        m_curBt = dynamic_cast<FillTextButton *>(sender());
        m_queryEdit->setText(QString(tr("Help me write an outline on the topic of [%1], which will be used for [PPT production].")).arg(topic.isEmpty() ? tr("Artificial Intelligence") : topic));
        m_queryEdit->moveCursor(QTextCursor::End);
        m_queryEdit->setFocus();
        ReportIns()->writeEvent(report::WriterFunctionPoint("outline").assemblingData());
        return;
    }

    if (sender() == m_notifyBt) {
        qCDebug(logWordWizard) << "click notify button";
        QString topic;
        if (m_curBt) {
            m_curBt->clearPressColor();
        } else {
            topic = m_queryEdit->toPlainText();
        }
        m_curBt = dynamic_cast<FillTextButton *>(sender());
        m_queryEdit->setText(QString(tr("Help me write a notice about [%1], the receiver is [All Employees] and the sender is [Administration Department].")).arg(topic.isEmpty() ? tr("National Day Holiday") : topic));
        m_queryEdit->moveCursor(QTextCursor::End);
        m_queryEdit->setFocus();
        ReportIns()->writeEvent(report::WriterFunctionPoint("notice").assemblingData());
        return;
    }

    if (sender() == m_reportBt) {
        qCDebug(logWordWizard) << "click report button";
        QString topic;
        if (m_curBt) {
            m_curBt->clearPressColor();
        } else {
            topic = m_queryEdit->toPlainText();
        }
        m_curBt = dynamic_cast<FillTextButton *>(sender());
        m_queryEdit->setText(QString(tr("Help me write a research report on [%1], at least it needs to include [status description, problem analysis, countermeasures and suggestions, research conclusions].")).arg(topic.isEmpty() ? tr("Artificial Intelligence") : topic));
        m_queryEdit->moveCursor(QTextCursor::End);
        m_queryEdit->setFocus();
        ReportIns()->writeEvent(report::WriterFunctionPoint("report").assemblingData());
        return;
    }

    if (sender() == m_speechBt) {
        qCDebug(logWordWizard) << "click speech button";
        QString topic;
        if (m_curBt) {
            m_curBt->clearPressColor();
        } else {
            topic = m_queryEdit->toPlainText();
        }
        m_curBt = dynamic_cast<FillTextButton *>(sender());
        m_queryEdit->setText(QString(tr("Help me write a speech on the topic of [%1] for [Company Leaders], requiring [clear structure and vivid language].")).arg(topic.isEmpty() ? tr("Artificial Intelligence") : topic));
        m_queryEdit->moveCursor(QTextCursor::End);
        m_queryEdit->setFocus();
        ReportIns()->writeEvent(report::WriterFunctionPoint("speech").assemblingData());
        return;
    }

    if (sender() == m_summaryBt) {
        qCDebug(logWordWizard) << "click summary button";
        QString topic;
        if (m_curBt) {
            m_curBt->clearPressColor();
        } else {
            topic = m_queryEdit->toPlainText();
        }
        m_curBt = dynamic_cast<FillTextButton *>(sender());
        m_queryEdit->setText(QString(tr("Help me write a summary of my recent work, including [%1] and [results], requiring a [formal] tone.")).arg(topic.isEmpty() ? tr("work content") : topic));
        m_queryEdit->moveCursor(QTextCursor::End);
        m_queryEdit->setFocus();
        ReportIns()->writeEvent(report::WriterFunctionPoint("summary").assemblingData());
        return;
    }

    if (sender() == m_publicBt) {
        qCDebug(logWordWizard) << "click public button";
        QString topic;
        if (m_curBt) {
            m_curBt->clearPressColor();
        } else {
            topic = m_queryEdit->toPlainText();
        }
        m_curBt = dynamic_cast<FillTextButton *>(sender());
        m_queryEdit->setText(QString(tr("Help me write a public tweet on the topic of [%1], requiring [clear structure] and [relaxed] tone.")).arg(topic.isEmpty() ? tr("Artificial Intelligence") : topic));
        m_queryEdit->moveCursor(QTextCursor::End);
        m_queryEdit->setFocus();
        ReportIns()->writeEvent(report::WriterFunctionPoint("tweet").assemblingData());
        return;
    }

    if (sender() == m_replaceBt) {
        qCInfo(logWordWizard) << "Replace button clicked, isFirstFill:" << m_isFirstFill;
        if (m_isFirstFill) { // 第一次使用填充功能，提醒用户重启输入法服务
            DDialog dlg(this);
            dlg.setWindowFlags(dlg.windowFlags() | Qt::WindowStaysOnTopHint);
            dlg.setMinimumWidth(380);
            dlg.setIcon(QIcon(":assets/images/tips.svg"));
            dlg.setMessage(tr("The function can be used only after restarting the input method. The tutorial is as follows:\n1. Right-click on the input method icon in the taskbar.\n2. Click \"Restart\"."));
            DLabel imageLabel(this);
            imageLabel.setPixmap(QPixmap(QIcon::fromTheme("uos-ai-assistant_fcitxrestart").pixmap(160, 160)));
            dlg.addContent(&imageLabel, Qt::AlignCenter);
            dlg.addButton(tr("Ok", "button"), true, DDialog::ButtonNormal);
            connect(&dlg, &DDialog::finished, this, [&] {
                qCDebug(logWordWizard) << "fcitx restart accepted";
                m_isFirstFill = false;
                DConfigManager::instance()->setValue(AIQUICK_GROUP, AIQUICK_ISFIRSTFILL, false);
            });
            dlg.exec();
        } else {
            this->setVisible(false);
            QTimer::singleShot(200, this, &AiWriterDialog::fillToText);
        }
        return;
    }

    if (sender() == m_againBt) {
        //qInfo() << QString("again");
        sendAiRequst(true);
        return;
    }

    if (sender() == m_copyBt) {
        this->copyText(m_reply);
        return;
    }

    if (sender() == m_cancelBt) {
        //qInfo() << QString("cancel");
        m_curErr = AiQuickDialog::ERROR_TYPE_NONE;
        this->enableReplyFunBt(true);

        m_reply        = m_replyBak;
        m_replyEditMax = m_replyEditMaxBak;
        m_replyEdit->setPlainText(m_reply);
        m_replyEdit->setVisible(true);
        m_errorInfoLabel->setVisible(false);
        m_cancelBt->setVisible(false);

        this->asyncAdjustSize();
        return;
    }
}

void AiWriterDialog::onTextChanged()
{
    if (m_queryEdit->toPlainText().isEmpty()) {
        this->clearArticleBtPressColor();
    }
    this->onQueryTextChanged();
}

void AiWriterDialog::onQueryTextChanged(bool isActiveChange) {
    m_queryEdit->blockSignals(true);

    QTextDocument *document = m_queryEdit->document();
    QTextCursor cursor(document);

    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    QColor defaultTextColor = parentPb.color(DPalette::Normal, DPalette::WindowText);
    QColor highTextColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
    if (!this->isActiveWindow()) {
        defaultTextColor = parentPb.color(DPalette::Inactive, DPalette::WindowText);
    }

    cursor.select(QTextCursor::Document);
    QTextCharFormat defaultFormat;
    defaultFormat.setForeground(defaultTextColor);
    cursor.mergeCharFormat(defaultFormat);

    QRegularExpressionMatchIterator matchIterator = m_queryEdit->highLightRegex.globalMatch(m_queryEdit->toPlainText());
    QTextCharFormat highFormat;
    highFormat.setForeground(highTextColor);

    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);

    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        cursor.setPosition(match.capturedStart());
        cursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
        cursor.setCharFormat(highFormat);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
    }
    cursor.endEditBlock();

    m_queryEdit->blockSignals(false);

    if (!isActiveChange) {
        this->asyncAdjustSize();
    }
}

void AiWriterDialog::onModelReply(int op, QString reply, int err) {
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
    if (!m_isShow) {
        return;
    }

    m_replyEdit->setVisible(true);
    m_inProgressLabel->setVisible(false);
    m_inProgressMovie->stop();
    m_progressVSpace->setVisible(false);

    /**
     * FREEACCOUNTEXPIRED = 9000,
     * FREEACCOUNTUSAGELIMIT = 9001,
     * FREEACCOUNTCHATUSAGELIMIT = 9002,
     * FREEACCOUNTTEXT2IMAGEUSAGELIMIT = 9003,
     * AccountInvalid = 9004,
     */
    if (err == AIServer::NoError) {
        m_curErr = AiQuickDialog::ERROR_TYPE_NONE;
        m_isReplyEnd = false;
        this->putAiReply(reply);
    } else if (err == 200) { // 流式回答结束符号
        m_isReplyEnd = true;
        this->putAiReply();
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
            // 其他错误的兜底处理，测试说：按正常回答处理
            m_curErr = AiQuickDialog::ERROR_TYPE_NONE;
            this->putAiReply(reply);
        }
    }

    this->asyncAdjustSize();
}

void AiWriterDialog::onFontChanged(const QFont &font) {
    this->asyncAdjustSize();
}

void AiWriterDialog::onUosAiLlmAccountLstChanged() {
    QString modelIdOrig = m_modelId;
    this->syncLlmAccount();
    // 账号列表变化了，但是首选model没变
    if (m_modelId == modelIdOrig) {
        return;
    }

    if (m_curErr != AiQuickDialog::ERROR_TYPE_NONE) {
        m_errorInfoLabel->setVisible(false);
        m_cancelBt->setVisible(false);

        this->sendAiRequst(true);
    }
}

void AiWriterDialog::onLlmAccountLstChanged(const QString &currentAccountId, const QString &accountLst) {
    this->onUosAiLlmAccountLstChanged();
}

void AiWriterDialog::onNetworkStateChanged(bool isOnline) {
    this->onUosAiLlmAccountLstChanged();
}

void AiWriterDialog::onOpenConfigDialog(const QString& link) {
    EAiExec()->launchLLMConfigWindow(false, false, false, tr("Model Configuration"));
    // BUG-292967，同步随航的方案：打开设置窗口后，关闭面板
    m_isNeedResetUi = true;
    this->close();
}

void AiWriterDialog::setQuerySepHeight(int height) {
    m_querySep->setFixedHeight(height);
}

void AiWriterDialog::showToast(const QString &message) {
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
    }

    QRect geometry(QPoint(10, 10), floatMessage->sizeHint());
    geometry.moveCenter(this->rect().center());

    //Show the message near bottom
    geometry.moveBottom(this->rect().bottom() - 8);
    floatMessage->setGeometry(geometry);
    floatMessage->show();
}

void AiWriterDialog::copyText(const QString &text) {
    //qInfo() << QString("copy");
    this->showToast(tr("Copied"));
    QApplication::clipboard()->setText(text);
}

void AiWriterDialog::syncLlmAccount() {
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
                m_modelInfo = tr("Current model: ") + infoJson["displayname"].toString();
                modelType = infoJson["type"].toInt();
                break;
            }
        }

        // m_modelId没找到，使用第一个
        if (!isFound) {
            isFound = true;
            QJsonObject infoJson = jsonArr[0].toObject();
            m_modelId   = infoJson["id"].toString();
            m_modelInfo = tr("Current model: ") + infoJson["displayname"].toString();
            modelType = infoJson["type"].toInt();
        }
    }

    if (!isFound) {
        m_modelId.clear();
        m_modelInfo = tr("Currently no model");
    }
    m_modelBt->setToolTip(m_modelInfo);

    this->asyncAdjustSize();
}

void AiWriterDialog::handleError() {
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
        m_replyEdit->setVisible(false);
        m_errorInfoLabel->setVisible(true);

        this->asyncAdjustSize();
        return;
    }
}

void AiWriterDialog::sendAiRequst(bool isAgain) {
    m_query = m_queryEdit->toPlainText();
    m_queryEdit->clear();
    m_queryEdit->setFirstQueryFlag(false);
    m_queryEdit->setPlaceholderText(tr("Modify the content, change the tone…"));
    m_queryEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_sendBt->setVisible(true);
    m_querySpace->setVisible(true);
    m_sendBt1->setVisible(false);
    m_sendVSpace->setVisible(false);

    int height = static_cast<int>(m_queryEdit->document()->size().height()) + 4;
    m_queryEdit->setFixedHeight(height);

    if (!m_reply.isEmpty()) {
        m_replyBak        = m_reply;
        m_replyEditMaxBak = m_replyEditMax;
    }
    m_reply.clear();
    m_replyEdit->clear();
    m_replyEditMax = this->maximumHeight();
    m_isReplyEditCursorEnd = true;

    m_replyCacheMutex.lock();
    m_replyCache.clear();
    m_replyCacheIdx = 0;
    m_replyCacheMutex.unlock();

    m_errorInfoLabel->setVisible(false);
    m_cancelBt->setVisible(false);
    m_replyEdit->setVisible(false);
    m_inProgressLabel->setVisible(true);
    m_inProgressMovie->start();
    m_progressVSpace->setVisible(true);

    // 每次发起请求时，隐藏下半部分控件
    this->enableReplyFunBt(false);
    this->setReplyPartVisible(false);
    this->setArticleBtPartVisible(false);

    // 每次发起请求时，复位错误码
    m_curErr = AiQuickDialog::ERROR_TYPE_NONE;
    EAiExec()->cancelAiRequst(m_reqId);
    EAiExec()->clearAiRequest(this);

    if (!isAgain) {
        m_prompt = m_query;
        if (!m_replyBak.isEmpty()) {
            m_prompt = QString("请按照要求【%1】对以下文本进行加工：\n%2").arg(m_query).arg(m_replyBak);
        }

        if (!m_queryTextEdit->isVisible()) {
            m_queryTextEdit->setFullText(m_prompt);
            m_querySep->setVisible(true);
            m_queryTextEdit->setVisible(true);
            m_queryHSep->setVisible(true);
            m_queryHSepVSpace->setVisible(true);
        }
    }
    qCInfo(logWordWizard) << "Sending AI request - model:" << m_modelInfo << "isAgain:" << isAgain << "query:" << m_prompt;
    m_reqId = EAiExec()->sendWordWizardRequest(m_modelId, m_prompt, this, QString("onModelReply"));

    this->asyncAdjustSize();
}

void AiWriterDialog::putAiReply(QString reply) {
    while (m_replyCache.isEmpty() && reply.startsWith("\n")) {
        reply.remove(0, 1);
    }
    reply.replace("\t", "    ");
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

void AiWriterDialog::smoothEachWord() {
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
        if (m_isReplyEnd && m_isShow) {
            this->setReplyPartVisible(true);
            this->enableReplyFunBt(true);
            this->asyncAdjustSize(100);
        }
        return;
    }
    m_replyCacheMutex.unlock();

    if (!m_isShow) {
        return;
    }

    m_replyEdit->insertPlainText(appendStr);
    this->asyncAdjustSize();

    QTimer::singleShot(30, this, &AiWriterDialog::smoothEachWord);
}

void AiWriterDialog::fillToText() {
    bool isWritable = WordWizard::fcitxWritable();
    qCInfo(logWordWizard) << "Replace button clicked, isWritable:" << isWritable;
    if (!isWritable) { // 此时，再次判断是否可写
        DDialog dlg(this);
        dlg.setWindowFlags(dlg.windowFlags() | Qt::WindowStaysOnTopHint);
        dlg.setIcon(QIcon(WARNING_ICON));
        dlg.setTitle(tr("Fill failed"));
        dlg.setMessage(tr("No input box selected, please select the input box and re-fill it."));
        dlg.addButton(tr("Cancel"), false, DDialog::ButtonNormal);
        dlg.addButton(tr("Ok"), true, DDialog::ButtonRecommend);
        connect(&dlg, &DDialog::accepted, this, [&] {
           //qInfo() << QString("fill accepted");
           m_isNeedResetUi = false;
           this->setVisible(true);
        });
        connect(&dlg, &DDialog::rejected, this, [&] {
           //qInfo() << QString("fill rejected, trigger this->close()");
           m_isNeedResetUi = true;
           this->close();
        });
        connect(&dlg, &DDialog::closed, this, [&] {
           //qInfo() << QString("fill closed, trigger this->close()");
           m_isNeedResetUi = true;
           this->close();
        });
        QPoint pos;
        pos.setX(this->geometry().x() + (this->sizeHint().width() - dlg.width()) / 2);
        pos.setY(this->geometry().y() + (this->sizeHint().height() - dlg.height()) / 2);
        dlg.move(pos);
        dlg.exec();
    } else {
        FcitxInputServer::getInstance().commitString(m_reply);
        //qInfo() << QString("after fill, trigger this->close()");
        m_isNeedResetUi = true;
        this->close();
    }
}

void AiWriterDialog::setArticleBtPartVisible(bool isTrue) {
    m_articleBt->setVisible(isTrue);
    m_outlineBt->setVisible(isTrue);
    m_notifyBt->setVisible(isTrue);
    m_reportBt->setVisible(isTrue);
    m_speechBt->setVisible(isTrue);
    m_summaryBt->setVisible(isTrue);
    m_publicBt->setVisible(isTrue);
    m_articleVSpace->setVisible(isTrue);
    m_articleVSpace1->setVisible(isTrue);

    if (isTrue) {
        this->clearArticleBtPressColor();
    }
}

void AiWriterDialog::clearArticleBtPressColor() {
    m_curBt = nullptr;

    m_articleBt->clearPressColor();
    m_outlineBt->clearPressColor();
    m_notifyBt->clearPressColor();
    m_reportBt->clearPressColor();
    m_speechBt->clearPressColor();
    m_summaryBt->clearPressColor();
    m_publicBt->clearPressColor();
}

void AiWriterDialog::enableReplyFunBt(bool isTrue) {
    m_replaceBt->setEnabled(isTrue);
    m_againBt->setEnabled(isTrue);
    m_copyBt->setEnabled(isTrue);
}

void AiWriterDialog::setReplyPartVisible(bool isTrue) {
    m_replaceBt->setVisible(isTrue);
    m_replySep->setVisible(isTrue);
    m_againBt->setVisible(isTrue);
    m_copyBt->setVisible(isTrue);

    m_vSpace->setVisible(isTrue);

    m_attentionLabel->setVisible(isTrue);
    m_modelBt->setVisible(isTrue);

    dynamic_cast<WriterTextEdit *>(m_replyEdit)->setInProgress(!isTrue);
}

void AiWriterDialog::adjustQueryEditSize() {
    if (!m_queryEdit->isFirstQuery()) { // 第一轮问答后，问题框锁定高度
        m_sendBt->setEnabled(!m_queryEdit->toPlainText().isEmpty());
        return;
    }

    int height = static_cast<int>(m_queryEdit->document()->size().height()) + 4;
    m_queryEdit->setFixedHeight(height > m_queryEditMax ? m_queryEditMax : height);

    if ((QFontMetrics(m_queryEdit->font()).horizontalAdvance(m_queryEdit->toPlainText()) > this->width() - 30 - m_sendBt->sizeHint().width() - 40)
            || m_queryEdit->toPlainText().indexOf("\n") != -1) {
        m_sendBt->setVisible(false);
        m_querySpace->setVisible(false);
        m_sendBt1->setVisible(true);
        m_sendVSpace->setVisible(true);
    } else {
        m_sendBt->setVisible(true);
        m_querySpace->setVisible(true);
        m_sendBt1->setVisible(false);
        m_sendVSpace->setVisible(false);
    }

    if (m_queryEdit->toPlainText().isEmpty()) {
        m_sendBt->setEnabled(false);
        this->adjustSize();
        return;
    }

    m_sendBt->setEnabled(true);

    this->adjustSize();

    if (m_queryEditMax == this->maximumHeight() && m_sendBt1->isVisible()) {
        m_queryEditMax = this->maximumHeight() - this->sizeHint().height() + m_queryEdit->height();
        m_queryEdit->setFixedHeight(height > m_queryEditMax ? m_queryEditMax : height);
    }
}

void AiWriterDialog::adjustReplyEditSize() {
    if (!m_replyEdit->isVisible()) {
        return;
    }

    QTextBlockFormat blockFmt = m_replyEdit->textCursor().blockFormat();
    blockFmt.setLineHeight(122, QTextBlockFormat::LineHeightTypes::ProportionalHeight);
    m_replyEdit->textCursor().setBlockFormat(blockFmt);

    int height = static_cast<int>(m_replyEdit->document()->size().height()) + 4;
    m_replyEdit->setFixedHeight(height > m_replyEditMax ? m_replyEditMax : height);
    this->adjustSize();

    if (this->sizeHint().height() + 50 > this->maximumHeight()
            && this->sizeHint().height() < this->maximumHeight()
            && m_replyEditMax == this->maximumHeight()) {
        m_replyEditMax = m_replyEdit->height() + (this->maximumHeight() - this->sizeHint().height());
    } else if (this->sizeHint().height() > this->maximumHeight()) {
        m_replyEditMax -= this->sizeHint().height() - this->maximumHeight();
        m_replyEdit->setFixedHeight(m_replyEditMax);
    } else if (this->sizeHint().height() < this->maximumHeight() && height > m_replyEditMax) {
        m_replyEditMax = m_replyEdit->height() + (this->maximumHeight() - this->sizeHint().height());
        m_replyEdit->setFixedHeight(height > m_replyEditMax ? m_replyEditMax : height);
    }
    this->adjustSize();

    if (m_isReplyEditCursorEnd) {
        m_replyEdit->moveCursor(QTextCursor::End);
    }
}

void AiWriterDialog::adjustDialogInitPosition() {
    static DDeDockObject dock(this);
    QRect frontendWindowRect = dock.frontendWindowRect();
    DockPosition dockPosition = static_cast<DockPosition>(dock.position());

    QList<QScreen *> screens = QGuiApplication::screens();
    QPoint cursorPos = QCursor::pos();
    QRect screenRect;

    qCDebug(logWordWizard) << "height:" << this->sizeHint().height();
    bool isFound = false;
    for (const QScreen *screen : screens) {
        if (screen->geometry().contains(cursorPos)) {
            screenRect = screen->geometry();
            isFound = true;
            break;
        }
    }
    if (!isFound && !screens.isEmpty()) {
        screenRect = screens[0]->geometry();
        isFound = true;
    }
    if (isFound) {
        QPoint pos;
        pos.setX(screenRect.x() + ((screenRect.width() - this->width()) / 2));
        if (dockPosition == DockPosition::Bottom) { // 任务栏在底下
            pos.setY(screenRect.bottom() - frontendWindowRect.height() - 50 - this->sizeHint().height());
        } else {
            pos.setY(screenRect.bottom() - 50 - this->sizeHint().height());
        }

        qCDebug(logWordWizard) << "screen:" << screenRect << "pos:" << pos;
        this->move(pos);
        m_initPos = pos;

#ifdef COMPILE_ON_V25
        if (ESystemContext::isTreeland()) {
            DDEShellWayland::get(windowHandle())->setPosition(m_initPos);
        }
#endif
    }

    this->syncLlmAccount();

    m_queryEdit->setFocus();
}

void AiWriterDialog::adjustDialogPosition() {
    int bottomLimit = 0;
    QList<QScreen *> screens = QGuiApplication::screens();
    for(QScreen *screen : screens) {
        if (screen->geometry().contains(this->geometry().x() + this->sizeHint().width() / 2, this->geometry().y())) {
            bottomLimit = screen->geometry().bottom();
            break;
        }
    }
    if (bottomLimit > 0 && this->geometry().y() + this->sizeHint().height() > bottomLimit) {
        // BUG-292883 控制中心开启最佳性能，坐标点有1个像素的偏移，需要纠正
        static int lastX = this->geometry().x();
        if (qAbs(lastX - this->geometry().x()) >= 2) {
            lastX = this->geometry().x();
        }
        this->move(lastX, this->geometry().y() - (this->geometry().y() + this->sizeHint().height() - bottomLimit));
    }
}

void AiWriterDialog::asyncAdjustSize(int ms) {
    QTimer::singleShot(ms, this, [&] {
        this->adjustQueryEditSize();
        this->adjustReplyEditSize();
        this->adjustSize();
        this->adjustDialogPosition();
    });
}
