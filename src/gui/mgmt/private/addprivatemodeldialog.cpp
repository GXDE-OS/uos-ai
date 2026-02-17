#include "addprivatemodeldialog.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "dbuscontrolcenterrequest.h"
#include "commonfaildialog.h"
#include "iconbuttonex.h"
#include "raidersbutton.h"
#include "uosfreeaccounts.h"
#include "gui/gutils.h"
#include "private/echatwndmanager.h"
#include "utils/util.h"
#include "utils/esystemcontext.h"

#include <DMessageManager>
#include <DPaletteHelper>

#include <QUuid>
#include <QLoggingCategory>

DCORE_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

AddPrivateModelDialog::AddPrivateModelDialog(DWidget *parent)
    : DAbstractDialog(parent)
{
    EWndManager()->registeWindow(this);

    initUI();
    initConnect();

    setModal(true);
    if (ESystemContext::isWayland()) {
        //wayland环境模态dialog需要手动置顶
        setWindowFlags(windowFlags() | Qt::Dialog |  Qt::WindowStaysOnTopHint);
    }
}

void AddPrivateModelDialog::initUI()
{
    setFixedWidth(518);
    //标题栏
    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);
    DFontSizeManager::instance()->bind(titleBar, DFontSizeManager::T5, QFont::DemiBold);
    titleBar->setTitle(tr("Add model"));

    DLabel *apiKeyLabel = new DLabel(tr("APIKey"));
    apiKeyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    apiKeyLabel->setToolTip(apiKeyLabel->text());
    DFontSizeManager::instance()->bind(apiKeyLabel, DFontSizeManager::T6, QFont::Medium);
    m_pApiKeyLineEdit = new DPasswordEdit(this);
    m_pApiKeyLineEdit->setPlaceholderText(tr("Required, please input"));
    m_pApiKeyLineEdit->setFixedWidth(403);

    DLabel *accountNameLabel = new DLabel(tr("Account"));
    accountNameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    accountNameLabel->setToolTip(accountNameLabel->text());
    DFontSizeManager::instance()->bind(accountNameLabel, DFontSizeManager::T6, QFont::Medium);
    m_pNameLineEdit = new DLineEdit();
    m_pNameLineEdit->setPlaceholderText(tr("Required, to distinguish multiple models"));
    m_pNameLineEdit->setFixedWidth(403);
    m_pNameLineEdit->lineEdit()->setMaxLength(21);
    m_pNameLineEdit->lineEdit()->installEventFilter(this);
    m_pNameLineEdit->lineEdit()->setProperty("_d_dtk_lineedit_opacity", false);

    DLabel *apiModelName = new DLabel(tr("Model Name"));
    apiModelName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    apiModelName->setToolTip(apiModelName->text());
    DFontSizeManager::instance()->bind(apiModelName, DFontSizeManager::T6, QFont::Medium);
    m_pCustomModelName = new DLineEdit();
    m_pCustomModelName->setPlaceholderText(tr("Optional"));
    m_pCustomModelName->lineEdit()->setMaxLength(64);

    DLabel *apiAddr = new DLabel(tr("Domain"));
    apiAddr->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    apiAddr->setToolTip(apiAddr->text());
    DFontSizeManager::instance()->bind(apiAddr, DFontSizeManager::T6, QFont::Medium);
    m_pCustomModelUrl = new DLineEdit();
    m_pCustomModelUrl->setPlaceholderText(tr("Required, please input"));
    m_pCustomModelUrl->lineEdit()->setMaxLength(512);

    m_pGridLayout = new QGridLayout();
    m_pGridLayout->setContentsMargins(0, 0, 0, 0);
    m_pGridLayout->setHorizontalSpacing(20);
    m_pGridLayout->setVerticalSpacing(10);
    m_pGridLayout->addWidget(accountNameLabel, 0, 0);
    m_pGridLayout->addWidget(m_pNameLineEdit, 0, 1);
    m_pGridLayout->addWidget(apiKeyLabel, 1, 0);
    m_pGridLayout->addWidget(m_pApiKeyLineEdit, 1, 1);
    m_pGridLayout->addWidget(apiModelName, 2, 0);
    m_pGridLayout->addWidget(m_pCustomModelName, 2, 1);
    m_pGridLayout->addWidget(apiAddr, 3, 0);
    m_pGridLayout->addWidget(m_pCustomModelUrl, 3, 1);

    QHBoxLayout *gridLayout = new QHBoxLayout();
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->addStretch();
    gridLayout->addLayout(m_pGridLayout);
    gridLayout->addStretch();

    DLabel *warningLabel = new DLabel(tr("To test whether the model is available, the system sends test information to the large model, which will consume a small amount of tokens."));
    warningLabel->setAlignment(Qt::AlignCenter);
    warningLabel->setFixedWidth(488);
    warningLabel->setWordWrap(true);
    DPalette pl = warningLabel->palette();
    QColor color = DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::BrightText);
    color.setAlphaF(0.5);
    pl.setColor(QPalette::Text, color);
    warningLabel->setPalette(pl);
    warningLabel->setForegroundRole(QPalette::Text);
    DFontSizeManager::instance()->bind(warningLabel, DFontSizeManager::T10, QFont::Medium);
    QHBoxLayout *warningLayout = new QHBoxLayout();
    warningLayout->addStretch();
    warningLayout->addWidget(warningLabel);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [ = ]() {
        DPalette pl = warningLabel->palette();
        QColor color = DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::BrightText);
        color.setAlphaF(0.5);
        pl.setColor(QPalette::Text, color);
        warningLabel->setPalette(pl);
    });

    m_pWidget = new QWidget(this);
    QVBoxLayout *lay = new QVBoxLayout(m_pWidget);
    lay->setContentsMargins(0, 5, 0, 10);
    lay->setSpacing(10);
    lay->addLayout(gridLayout);
    lay->addLayout(warningLayout);
    lay->addStretch();

    // 代理区域
    QHBoxLayout *proxyLayout = new QHBoxLayout();
    proxyLayout->setContentsMargins(0, 0, 0, 0);
    proxyLayout->setSpacing(5);
    m_pProxyLabel = new DLabel();
    m_pProxyLabel->setFixedWidth(488);
    m_pProxyLabel->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_pProxyLabel, DFontSizeManager::T6, QFont::Medium);
    m_pProxyLabel->setWordWrap(true);
    proxyLayout->addWidget(m_pProxyLabel);

    // 按钮区域
    m_pCancelButton = new DPushButton(tr("Cancel"));
    m_pCancelButton->setFixedWidth(200);
    m_pSubmitButton = new DSuggestButton(tr("Confirm"));
    m_pSubmitButton->setFixedWidth(200);
    m_pSubmitButton->setEnabled(false);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_pCancelButton);
    buttonLayout->addWidget(m_pSubmitButton);
    buttonLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 10);
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(m_pWidget);
    mainLayout->addStretch();
    mainLayout->addLayout(proxyLayout);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    m_pSpinner = new DSpinner(this);
    m_pSpinner->setFixedSize(32, 32);
    m_pSpinner->hide();
    m_pSpinner->move(243, GUtils::isCompactMode() ? 120 : 156);

    m_widgetSet.insert(apiKeyLabel);
    m_widgetSet.insert(accountNameLabel);

    m_widgetSet.insert(m_pApiKeyLineEdit);
    m_widgetSet.insert(m_pNameLineEdit);
    m_widgetSet.insert(m_pCancelButton);
    m_widgetSet.insert(m_pSubmitButton);

    updateProxyLabel();
    onCompactModeChanged();
}

void AddPrivateModelDialog::initConnect()
{
    connect(m_pCancelButton, &DPushButton::clicked, this, &AddPrivateModelDialog::reject);
    connect(m_pSubmitButton, &DPushButton::clicked, this, &AddPrivateModelDialog::onSubmitButtonClicked);
    connect(m_pApiKeyLineEdit, &DPasswordEdit::textChanged, this, &AddPrivateModelDialog::onUpdateSubmitButtonStatus);

    connect(m_pCustomModelName, &DPasswordEdit::textChanged, this, &AddPrivateModelDialog::onUpdateSubmitButtonStatus);
    connect(m_pCustomModelUrl, &DPasswordEdit::textChanged, this, &AddPrivateModelDialog::onUpdateSubmitButtonStatus);

    connect(m_pNameLineEdit, &DLineEdit::textChanged, this, &AddPrivateModelDialog::onNameTextChanged);
    connect(m_pNameLineEdit, &DLineEdit::alertChanged, this, &AddPrivateModelDialog::onNameAlertChanged);

    GUtils::connection(DGuiApplicationHelper::instance(), "sizeModeChanged(DGuiApplicationHelper::SizeMode)", this, "onCompactModeChanged()");
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));
    connect(m_pProxyLabel, &DLabel::linkActivated, this, []() {
        DbusControlCenterRequest dbus;
#ifdef COMPILE_ON_V23
        dbus.showPage("network/systemProxy");
#else
        dbus.showPage("network", "System Proxy");
#endif
    });
}

void AddPrivateModelDialog::setAllWidgetEnabled(bool enable)
{
    if (enable) {
        m_pSpinner->hide();
        m_pSpinner->stop();
    } else {
        m_pSpinner->start();
        m_pSpinner->show();
    }
    for (auto w : m_widgetSet)
        w->setEnabled(enable);
}

void AddPrivateModelDialog::onNameTextChanged(const QString &str)
{
    QString noEnterText = str;
    if (noEnterText.contains("\n")) {
        noEnterText.replace("\n", " ");
        int cursorPos = m_pNameLineEdit->lineEdit()->cursorPosition();
        m_pNameLineEdit->setText(noEnterText);
        m_pNameLineEdit->lineEdit()->setCursorPosition(cursorPos);
    }
    m_pNameLineEdit->setAlert(false);
    m_pNameLineEdit->hideAlertMessage();

    if (noEnterText.length() > 20) {
        m_pNameLineEdit->blockSignals(true);
        m_pNameLineEdit->showAlertMessage(tr("No more than 20 characters"));
        m_pNameLineEdit->setText(noEnterText.left(20));
        m_pNameLineEdit->blockSignals(false);
    }
    onUpdateSubmitButtonStatus();
}

void AddPrivateModelDialog::onNameAlertChanged(bool alert)
{
    if (alert)
        m_pSubmitButton->setDisabled(true);
}

void AddPrivateModelDialog::onSubmitButtonClicked()
{
    qCInfo(logAIGUI) << "Submit button clicked";
    auto llmList = DbWrapper::localDbWrapper().queryLlmList(true);
    if (isNameDuplicate(llmList)) {
        qCWarning(logAIGUI) << "Duplicate model name detected:" << m_pNameLineEdit->text();
        return;
    }

    QString apiKey = m_pApiKeyLineEdit->text().trimmed();
    QString modelName = m_pCustomModelName->text().trimmed();
    QString url = m_pCustomModelUrl->text().trimmed();

    for (auto llm : llmList) {
        if (llm.model == LLMChatModel::PRIVATE_MODEL && llm.account.apiKey == apiKey && llm.ext.value(LLM_EXTKEY_VENDOR_MODEL) == modelName) {
            if (llm.ext.value(LLM_EXTKEY_VENDOR_URL) == url){
                qCWarning(logAIGUI) << "LLM already exists, aborting add. Name:" << m_pNameLineEdit->text();
                CommonFailDialog dlg(this);
                dlg.setFailMsg(tr("This LLM already exists, please do not add it again."));
                dlg.exec();
                return;
            }
        }
    }

    LLMChatModel model = LLMChatModel::PRIVATE_MODEL;

    LLMServerProxy serverProxy;
    serverProxy.name = m_pNameLineEdit->text();
    serverProxy.id = QUuid::createUuid().toString(QUuid::Id128);
    serverProxy.model = model;

    AccountProxy accountProxy;
    SocketProxy socketProxy;
    socketProxy.socketProxyType = SocketProxyType::SYSTEM_PROXY;
    accountProxy.socketProxy = socketProxy;
    accountProxy.apiKey = apiKey;
    serverProxy.account = accountProxy;
    serverProxy.type = ModelType::USER; //设置默认类型，防止未初始化就使用

    serverProxy.ext.insert(LLM_EXTKEY_VENDOR_MODEL, modelName);
    serverProxy.ext.insert(LLM_EXTKEY_VENDOR_URL, url);

    m_data = serverProxy;

    setAllWidgetEnabled(false);
    qCInfo(logAIGUI) << "Verifying private LLM server proxy before saving.";
    QPair<int, QString> pair = ServerWrapper::instance()->verify(serverProxy);
    setAllWidgetEnabled(true);

    if (0 != pair.first) {
        qCCritical(logAIGUI) << "LLM server proxy verification failed. Error:" << pair.second;
        CommonFailDialog dlg(this);
        dlg.setFailMsg(pair.second);
        dlg.exec();
        return;
    }

    if (!DbWrapper::localDbWrapper().appendLlm(serverProxy)) {
        qCCritical(logAIGUI) << "Failed to save LLM to database.";
        CommonFailDialog dlg(this);
        dlg.setFailMsg(tr("Save failed, please try again later"));
        dlg.exec();
        return;
    }

    qCInfo(logAIGUI) << "Private LLM added successfully.";
    ServerWrapper::instance()->updateLLMAccount();
    this->accept();
}

LLMServerProxy AddPrivateModelDialog::getModelData()
{
    return m_data;
}

void AddPrivateModelDialog::onUpdateSubmitButtonStatus()
{
    bool disable = true;

    disable = m_pNameLineEdit->text().isEmpty() || m_pCustomModelUrl->text().isEmpty();

    m_pSubmitButton->setDisabled(disable);
}

bool AddPrivateModelDialog::event(QEvent *event)
{
    // 网络请求中，屏蔽关闭按钮和esc快捷键功能
    if (event->type() == QEvent::KeyPress) {
        auto keyEvent = dynamic_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape && m_pSpinner->isPlaying()) {
            event->ignore();
            return true;
        }
    } else if (event->type() == QEvent::Close) {
        if (m_pSpinner->isPlaying()) {
            event->ignore();
            return true;
        }
    } else if (event->type() == QEvent::PaletteChange) {
        QMetaObject::invokeMethod(this, "updateProxyLabel", Qt::QueuedConnection);
    }

    return DAbstractDialog::event(event);
}

bool AddPrivateModelDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_pNameLineEdit->lineEdit() && event->type() == QEvent::FocusOut) {
        isNameDuplicate(DbWrapper::localDbWrapper().queryLlmList(true));
    }
    return DAbstractDialog::eventFilter(watched, event);
}

bool AddPrivateModelDialog::isNameDuplicate(const QList<LLMServerProxy> &llmList) const
{
    for (auto llm : llmList) {
        if (llm.name.trimmed() == m_pNameLineEdit->text().trimmed()) {
            m_pNameLineEdit->setAlert(true);
            m_pNameLineEdit->showAlertMessage(tr("The account name already exists, please change it"), -1);
            return true;
        }
    }
    return false;
}

void AddPrivateModelDialog::onCompactModeChanged()
{
    int height = GUtils::isCompactMode() ? 24 : 36;
    for (auto w : m_widgetSet) {
        if (w) w->setFixedHeight(height);
    }

    m_pWidget->setMinimumHeight(GUtils::isCompactMode() ? 190 : 220);
    m_pSpinner->move(243, GUtils::isCompactMode() ? 120 : 156);
    this->adjustSize();
}

void AddPrivateModelDialog::onUpdateSystemFont(const QFont &)
{
    updateProxyLabel();
    this->adjustSize();
}

void AddPrivateModelDialog::updateProxyLabel()
{
    const QColor &color = DPaletteHelper::instance()->palette(m_pProxyLabel).color(DPalette::Normal, DPalette::Highlight);
    m_pProxyLabel->setText(tr("For proxy settings, please go to system proxy settings")
                           + QString("<a href=\"javascript:void(0)\" style=\"color:%1; text-decoration: none;\"> %2<img src=\"%3\"></a>")
                           .arg(color.name())
                           .arg(tr("Go to settings"))
                           .arg(GUtils::generateImage(m_pProxyLabel, color, QSize(m_pProxyLabel->font().pixelSize() - 4, m_pProxyLabel->font().pixelSize() - 4), QStyle::PE_IndicatorArrowRight))
                          );
}

void AddPrivateModelDialog::resetDialog()
{
    m_pApiKeyLineEdit->clear();
    m_pNameLineEdit->clear();
    m_pCustomModelName->clear();
    m_pCustomModelUrl->clear();
}
