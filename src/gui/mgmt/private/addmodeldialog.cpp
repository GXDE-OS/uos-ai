#include "addmodeldialog.h"
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

#include <DPaletteHelper>
#include <DMessageManager>

#include <QUuid>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE
DCORE_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

AddModelDialog::AddModelDialog(DWidget *parent)
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

void AddModelDialog::initUI()
{
    setFixedWidth(528);

    //标题栏
    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);
    DFontSizeManager::instance()->bind(titleBar, DFontSizeManager::T5, QFont::DemiBold);
    titleBar->setTitle(tr("Add model"));

    DLabel *modelLabel = new DLabel(tr("LLM"));
    modelLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    modelLabel->setToolTip(modelLabel->text());
    DFontSizeManager::instance()->bind(modelLabel, DFontSizeManager::T6, QFont::Medium);

    m_openrouterModelNameLst.clear();
    m_openrouterModelIDLst.clear();

    m_pModelComboBox = new DComboBox();
#ifndef QT_DEBUG
    if (Util::isGPTEnable()) {
        m_modelMap.insert(m_modelMap.size(), LLMChatModel::CHATGPT_3_5_16K);
        m_modelMap.insert(m_modelMap.size(), LLMChatModel::CHATGPT_4);
        m_modelMap.insert(m_modelMap.size(), LLMChatModel::GEMINI_1_5_FLASH);
        m_modelMap.insert(m_modelMap.size(), LLMChatModel::GEMINI_1_5_PRO);
        // openrouter model
        m_modelMap.insert(m_modelMap.size(), LLMChatModel::OPENROUTER_MODEL);
    }
#else
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::CHATGPT_3_5_16K);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::CHATGPT_4);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::GEMINI_1_5_FLASH);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::GEMINI_1_5_PRO);
    // openrouter model
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::OPENROUTER_MODEL);
#endif
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::WXQF_ERNIE_Bot);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::SPARKDESK);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::GPT360_S2_V9);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::ChatZHIPUGLM_PRO);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::DeepSeek_R1);
    // custom model
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::OPENAI_API_COMPATIBLE);

    for (int i = 0; i < m_modelMap.size(); i++) {
        m_pModelComboBox->addItem(LLMServerProxy::llmName(m_modelMap.value(i), true));
    }
    m_pModelComboBox->setFixedWidth(381);
    m_threeKeyComboxIndex.insert(WXQF_ERNIE_Bot);
    m_threeKeyComboxIndex.insert(SPARKDESK);

    DLabel *appIdLabel = new DLabel(tr("APPID"));
    appIdLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    appIdLabel->setToolTip(appIdLabel->text());
    DFontSizeManager::instance()->bind(appIdLabel, DFontSizeManager::T6, QFont::Medium);
    m_pAppIdLineEdit = new DPasswordEdit(this);
    m_pAppIdLineEdit->setPlaceholderText(tr("Required, please input"));
    m_pAppIdLineEdit->setFixedWidth(381);

    DLabel *apiKeyLabel = new DLabel(tr("APIKey"));
    apiKeyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    apiKeyLabel->setToolTip(apiKeyLabel->text());
    DFontSizeManager::instance()->bind(apiKeyLabel, DFontSizeManager::T6, QFont::Medium);
    m_pApiKeyLineEdit = new DPasswordEdit(this);
    m_pApiKeyLineEdit->setPlaceholderText(tr("Required, please input"));
    m_pApiKeyLineEdit->setFixedWidth(381);

    DLabel *apiSecretLabel = new DLabel(tr("APISecret"));
    apiSecretLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    apiSecretLabel->setToolTip(apiSecretLabel->text());
    DFontSizeManager::instance()->bind(apiSecretLabel, DFontSizeManager::T6, QFont::Medium);
    m_pApiSecretLineEdit = new DPasswordEdit(this);
    m_pApiSecretLineEdit->setPlaceholderText(tr("Required, please input"));
    m_pApiSecretLineEdit->setFixedWidth(381);

    DLabel *accountNameLabel = new DLabel(tr("Account"));
    accountNameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    accountNameLabel->setToolTip(accountNameLabel->text());
    DFontSizeManager::instance()->bind(accountNameLabel, DFontSizeManager::T6, QFont::Medium);
    m_pNameLineEdit = new DLineEdit();
    m_pNameLineEdit->setPlaceholderText(tr("Required, to distinguish multiple models"));
    m_pNameLineEdit->setFixedWidth(381);
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

    DLabel *requestAddressLabel = new DLabel(tr("Domain"));
    requestAddressLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    requestAddressLabel->setToolTip(requestAddressLabel->text());
    DFontSizeManager::instance()->bind(requestAddressLabel, DFontSizeManager::T6, QFont::Medium);
    m_pAddressLineEdit = new DLineEdit();
    m_pAddressLineEdit->setPlaceholderText(tr("Optional. The default address will be used if not filled in.")); // 非必填，未填写时将使用默认地址
    m_pAddressLineEdit->setFixedWidth(381);
    m_pAddressLineEdit->lineEdit()->setMaxLength(256);
    m_pAddressLineEdit->lineEdit()->installEventFilter(this);
    m_pAddressLineEdit->lineEdit()->setProperty("_d_dtk_lineedit_opacity", false);

    DLabel *modelLstLabel = new DLabel(tr("Models List"));
    modelLstLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    modelLstLabel->setToolTip(requestAddressLabel->text());
    DFontSizeManager::instance()->bind(modelLstLabel, DFontSizeManager::T6, QFont::Medium);

    m_modelLstComboBox = new DComboBox();
    m_modelLstComboBox->setFixedWidth(381);
    m_modelLstComboBox->addItem(tr("Custom"));

    m_pGridLayout = new QGridLayout();
    m_pGridLayout->setContentsMargins(0, 0, 0, 0);
    m_pGridLayout->setHorizontalSpacing(20);
    m_pGridLayout->setVerticalSpacing(10);
    m_pGridLayout->addWidget(accountNameLabel, 0, 0);
    m_pGridLayout->addWidget(m_pNameLineEdit, 0, 1);
    m_pGridLayout->addWidget(modelLabel, 1, 0);
    m_pGridLayout->addWidget(m_pModelComboBox, 1, 1);
    m_pGridLayout->addWidget(appIdLabel, 2, 0);
    m_pGridLayout->addWidget(m_pAppIdLineEdit, 2, 1);
    m_pGridLayout->addWidget(apiKeyLabel, 3, 0);
    m_pGridLayout->addWidget(m_pApiKeyLineEdit, 3, 1);
    m_pGridLayout->addWidget(apiSecretLabel, 4, 0);
    m_pGridLayout->addWidget(m_pApiSecretLineEdit, 4, 1);

    m_pGridLayout->addWidget(apiModelName, 5, 0);
    m_pGridLayout->addWidget(m_pCustomModelName, 5, 1);
    m_pGridLayout->addWidget(apiAddr, 6, 0);
    m_pGridLayout->addWidget(m_pCustomModelUrl, 6, 1);

    m_pGridLayout->addWidget(requestAddressLabel, 7, 0);
    m_pGridLayout->addWidget(m_pAddressLineEdit, 7, 1);

    m_pGridLayout->addWidget(modelLstLabel, 8, 0);
    m_pGridLayout->addWidget(m_modelLstComboBox, 8, 1);

    QHBoxLayout *gridLayout = new QHBoxLayout();
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->addStretch();
    gridLayout->addLayout(m_pGridLayout);
    gridLayout->addStretch();

    m_warningLabel = new DLabel(tr("To test whether the model is available, the system sends test information to the large model, which will consume a small amount of tokens."));
    m_warningLabel->setAlignment(Qt::AlignCenter);
    m_warningLabel->setFixedWidth(488);
    m_warningLabel->setWordWrap(true);
    DPalette pl = m_warningLabel->palette();
    QColor color = DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::BrightText);
    color.setAlphaF(0.5);
    pl.setColor(QPalette::Text, color);
    m_warningLabel->setPalette(pl);
    m_warningLabel->setForegroundRole(QPalette::Text);
    DFontSizeManager::instance()->bind(m_warningLabel, DFontSizeManager::T10, QFont::Medium);
    QHBoxLayout *warningLayout = new QHBoxLayout();
    warningLayout->addStretch();
    warningLayout->addWidget(m_warningLabel);
    warningLayout->addStretch();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [ = ]() {
        DPalette pl = m_warningLabel->palette();
        QColor color = DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::BrightText);
        color.setAlphaF(0.5);
        pl.setColor(QPalette::Text, color);
        m_warningLabel->setPalette(pl);
    });

    m_pWidget = new QWidget(this);
    QVBoxLayout *lay = new QVBoxLayout(m_pWidget);
    lay->setContentsMargins(0, 5, 0, 10);
    lay->setSpacing(10);
    lay->addLayout(gridLayout);
    lay->addLayout(warningLayout);
    lay->addStretch();

    // 攻略区域
    m_pRaidersButton = new RaidersButton;
    DWidget *raidersWidget = new DWidget;
    raidersWidget->setFixedHeight(49);
    QHBoxLayout *raidersLayout = new QHBoxLayout(raidersWidget);
    raidersLayout->setContentsMargins(0, 8, 0, 7);
    raidersLayout->addWidget(m_pRaidersButton);
    raidersLayout->addStretch();

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
    mainLayout->addWidget(raidersWidget);
    mainLayout->addLayout(proxyLayout);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    m_pSpinner = new DSpinner(this);
    m_pSpinner->setFixedSize(32, 32);
    m_pSpinner->hide();
    //m_pSpinner->move(243, GUtils::isCompactMode() ? 120 : 156);

    onComboBoxIndexChanged(m_pModelComboBox->currentIndex());

    m_widgetSet.insert(modelLabel);
    m_widgetSet.insert(appIdLabel);
    m_widgetSet.insert(apiKeyLabel);
    m_widgetSet.insert(apiSecretLabel);
    m_widgetSet.insert(accountNameLabel);
    m_widgetSet.insert(requestAddressLabel);

    m_widgetSet.insert(m_pModelComboBox);
    m_widgetSet.insert(m_pAppIdLineEdit);
    m_widgetSet.insert(m_pApiKeyLineEdit);
    m_widgetSet.insert(m_pApiSecretLineEdit);
    m_widgetSet.insert(m_pNameLineEdit);
    m_widgetSet.insert(m_pCancelButton);
    m_widgetSet.insert(m_pSubmitButton);
    m_widgetSet.insert(m_pAddressLineEdit);

    updateProxyLabel();
    onCompactModeChanged();
}

void AddModelDialog::initConnect()
{
    connect(m_pModelComboBox, QOverload<int>::of(&DComboBox::currentIndexChanged), this, &AddModelDialog::onComboBoxIndexChanged);
    connect(m_pCancelButton, &DPushButton::clicked, this, &AddModelDialog::reject);
    connect(m_pSubmitButton, &DPushButton::clicked, this, &AddModelDialog::onSubmitButtonClicked);
    connect(m_pAppIdLineEdit, &DPasswordEdit::textChanged, this, &AddModelDialog::onUpdateSubmitButtonStatus);
    connect(m_pApiKeyLineEdit, &DPasswordEdit::textChanged, this, &AddModelDialog::onUpdateSubmitButtonStatus);
    connect(m_pApiSecretLineEdit, &DPasswordEdit::textChanged, this, &AddModelDialog::onUpdateSubmitButtonStatus);

    connect(m_pCustomModelName, &DPasswordEdit::textChanged, this, &AddModelDialog::onUpdateSubmitButtonStatus);
    connect(m_pCustomModelUrl, &DPasswordEdit::textChanged, this, &AddModelDialog::onUpdateSubmitButtonStatus);

    connect(m_pNameLineEdit, &DLineEdit::textChanged, this, &AddModelDialog::onNameTextChanged);
    connect(m_pNameLineEdit, &DLineEdit::alertChanged, this, &AddModelDialog::onNameAlertChanged);
    connect(m_pAddressLineEdit, &DLineEdit::textChanged, this, &AddModelDialog::onAddressTextChanged);
    connect(m_pAddressLineEdit, &DLineEdit::alertChanged, this, &AddModelDialog::onAddressAlertChanged);
    connect(m_modelLstComboBox, QOverload<int>::of(&DComboBox::currentIndexChanged), this, &AddModelDialog::onOpenRouterIndexChanged);

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

void AddModelDialog::setAllWidgetEnabled(bool enable)
{
    if (enable) {
        m_pSpinner->hide();
        m_pSpinner->stop();
    } else {
        int y = (m_pWidget->y() + m_warningLabel->y() + m_warningLabel->height() + m_pProxyLabel->y()) / 2 - m_pSpinner->height() / 2;
        m_pSpinner->move(243, y);
        m_pSpinner->start();
        m_pSpinner->show();
    }
    for (auto w : m_widgetSet)
        w->setEnabled(enable);
}

void AddModelDialog::onNameTextChanged(const QString &str)
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

void AddModelDialog::onNameAlertChanged(bool alert)
{
    if (alert)
        m_pSubmitButton->setDisabled(true);
}

void AddModelDialog::onAddressTextChanged(const QString &)
{
    onUpdateSubmitButtonStatus();
}

void AddModelDialog::onAddressAlertChanged(bool alert)
{

}

void AddModelDialog::onSubmitButtonClicked()
{
    qCInfo(logAIGUI) << "Submit button clicked";
    auto llmList = DbWrapper::localDbWrapper().queryLlmList(true);
    if (isNameDuplicate(llmList)) {
        qCWarning(logAIGUI) << "Duplicate model name detected:" << m_pNameLineEdit->text();
        return;
    }

    LLMChatModel model = m_modelMap.value(m_pModelComboBox->currentIndex());
    QString apiKey = m_pApiKeyLineEdit->text().trimmed();
    QString appId = m_pAppIdLineEdit->text().trimmed();
    QString apiSecret = m_pApiSecretLineEdit->text().trimmed();
    QString requestAddress = m_pAddressLineEdit->text().trimmed();
    QString modelName = m_pCustomModelName->text().trimmed();
    QString url = m_pCustomModelUrl->text().trimmed();

    bool isKol = false;
    if (m_threeKeyComboxIndex.contains(model)) {
        //解密成功才可能是kol账号
        UosAccountEncoder encoder;
        isKol = !std::get<0>(encoder.decrypt(appId)).isEmpty()
                        && !std::get<0>(encoder.decrypt(apiKey)).isEmpty()
                        && !std::get<0>(encoder.decrypt(apiSecret)).isEmpty();
    } else if (model == DeepSeek_R1){
        UosAccountEncoder encoder;
        isKol = !std::get<0>(encoder.decrypt(apiKey)).isEmpty();
    }

    bool exist = false;
    for (auto llm : llmList) {
        if (llm.model != model)
            continue;

        // gpt 360 zhipu，只判断key重复
        if ((
                (llm.model >= CHATGPT_3_5 && llm.model <= CHATGPT_4_32K)
                || (llm.model == GPT360_S2_V9)
                || (llm.model >= ChatZHIPUGLM_PRO && llm.model <= ChatZHIPUGLM_LITE)
                || (llm.model >= GEMINI_1_5_FLASH && llm.model <= GEMINI_1_5_PRO)
                || (llm.model == DeepSeek_R1)
             ) && (llm.account.apiKey == apiKey)) {
            exist = true;
            break;
        }

        // 讯飞、百度，判断key id secret重复
        if (m_threeKeyComboxIndex.contains(llm.model)
                && llm.account.appId == appId && llm.account.apiKey == apiKey && llm.account.apiSecret == apiSecret) {
            if (isKol) { // kol账号不检查 requestAddress
                exist = true;
                break;
            } else if (llm.url == requestAddress){
                exist = true;
                break;
            }
        }

        // 自定义模型判断apikey 模型名 请求地址重复
        if (llm.model == LLMChatModel::OPENAI_API_COMPATIBLE && llm.account.apiKey == apiKey && llm.ext.value(LLM_EXTKEY_VENDOR_MODEL) == modelName) {
            if (llm.ext.value(LLM_EXTKEY_VENDOR_URL) == url){
                exist = true;
                break;
            }
        }
    }

    if (exist) {
        qCWarning(logAIGUI) << "LLM already exists, aborting add. Model:" << model << ", Name:" << m_pNameLineEdit->text();
        CommonFailDialog dlg(this);
        dlg.setFailMsg(tr("This LLM already exists, please do not add it again."));
        dlg.exec();
        return;
    }

    //验证KOL账号
    if (isKol) {
        qCInfo(logAIGUI) << "Verifying KOL account for model:" << model;
        UosFreeAccount freeAccount;
        freeAccount.appid = appId;
        freeAccount.appkey = apiKey;
        freeAccount.appsecret = apiSecret;
        int status;

        setAllWidgetEnabled(false);
        QNetworkReply::NetworkError error = UosFreeAccounts::instance().
                getFreeAccount(ModelType::FREE_KOL, model == DeepSeek_R1 ? UOS_FREE : NoModel, freeAccount, status);
        setAllWidgetEnabled(true);

        if (QNetworkReply::NoError == error) {
            qCInfo(logAIGUI) << "KOL account verified successfully.";
            LLMChatModel m = static_cast<LLMChatModel>(freeAccount.llmModel);
            bool typeError = false; //模型错误，百度两种模型账号可以混用，但是百度和讯飞不能混用
            // 选择正确的时候
            if (m != model) {
                if (m == LLMChatModel::WXQF_ERNIE_Bot) {
                    m = model;
                } else if (m == UOS_FREE && model == DeepSeek_R1) {
                    model = UOS_FREE;
                } else {
                    typeError = true;
                    qCWarning(logAIGUI) << "KOL account type mismatch. Expected:" << model << ", Got:" << m;
                }
            }

            if (!typeError) {
                //是kol账号添加到界面去
                LLMServerProxy llm;
                llm.type = ModelType::FREE_KOL;
                llm.name = m_pNameLineEdit->text();
                llm.id = freeAccount.appkey + "_" + QChar(model);
                llm.model = m;
                llm.url = freeAccount.modelUrl;
                AccountProxy accountProxy;
                SocketProxy socketProxy;
                socketProxy.socketProxyType = SocketProxyType::SYSTEM_PROXY;
                accountProxy.socketProxy = socketProxy;
                accountProxy.appId = freeAccount.appid;
                accountProxy.apiKey = freeAccount.appkey;
                accountProxy.apiSecret = freeAccount.appsecret;
                llm.account = accountProxy;
                m_data = llm;

                if (!DbWrapper::localDbWrapper().appendLlm(llm)) {
                    qCCritical(logAIGUI) << "Failed to save KOL LLM to database.";
                    CommonFailDialog dlg(this);
                    dlg.setFailMsg(tr("Save failed, please try again later"));
                    dlg.exec();
                    return;
                }

                qCInfo(logAIGUI) << "KOL LLM added successfully.";
                ServerWrapper::instance()->updateLLMAccount();
                this->accept();
                return;
            }
        } else if (1 != status && 3 != status) {
            qCCritical(logAIGUI) << "Network/server error when verifying KOL account. Status:" << status;
            CommonFailDialog dlg(this);
            dlg.setFailMsg(tr("Unable to connect to the server, please check your network or try again later."));
            dlg.exec();
            return;
        }
        // 其他场景，说明不是kol账号，走正常添加流程
    }

    LLMServerProxy serverProxy;
    serverProxy.name = m_pNameLineEdit->text();
    serverProxy.id = QUuid::createUuid().toString(QUuid::Id128);
    serverProxy.model = model;
    serverProxy.url = requestAddress;

    AccountProxy accountProxy;
    SocketProxy socketProxy;
    socketProxy.socketProxyType = SocketProxyType::SYSTEM_PROXY;
    accountProxy.socketProxy = socketProxy;
    accountProxy.appId = appId;
    accountProxy.apiKey = apiKey;
    accountProxy.apiSecret = apiSecret;
    serverProxy.account = accountProxy;
    serverProxy.type = ModelType::USER; //设置默认类型，防止未初始化就使用

    if (model == LLMChatModel::OPENAI_API_COMPATIBLE) {
        serverProxy.ext.insert(LLM_EXTKEY_VENDOR_MODEL, modelName);
        serverProxy.ext.insert(LLM_EXTKEY_VENDOR_URL, url);
    }

    else if (model == LLMChatModel::OPENROUTER_MODEL) {
        serverProxy.ext.insert(LLM_EXTKEY_VENDOR_MODEL, modelName);
        serverProxy.ext.insert(LLM_EXTKEY_VENDOR_URL, "https://openrouter.ai/api/v1");
    }

    m_data = serverProxy;

    setAllWidgetEnabled(false);
    qCInfo(logAIGUI) << "Verifying LLM server proxy before saving. Model:" << model;
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

    qCInfo(logAIGUI) << "LLM added successfully.";
    ServerWrapper::instance()->updateLLMAccount();
    this->accept();
}

LLMServerProxy AddModelDialog::getModelData()
{
    return m_data;
}

void AddModelDialog::onComboBoxIndexChanged(int index)
{
    m_pGridLayout->itemAtPosition(8, 0)->widget()->hide();
    m_pGridLayout->itemAtPosition(8, 1)->widget()->hide();

    LLMChatModel model = m_modelMap.value(index);
    if (!m_threeKeyComboxIndex.contains(model)) {
        m_pGridLayout->itemAtPosition(2, 0)->widget()->hide();
        m_pGridLayout->itemAtPosition(2, 1)->widget()->hide();
        m_pGridLayout->itemAtPosition(4, 0)->widget()->hide();
        m_pGridLayout->itemAtPosition(4, 1)->widget()->hide();
    } else {
        m_pGridLayout->itemAtPosition(2, 0)->widget()->show();
        m_pGridLayout->itemAtPosition(2, 1)->widget()->show();
        m_pGridLayout->itemAtPosition(4, 0)->widget()->show();
        m_pGridLayout->itemAtPosition(4, 1)->widget()->show();
    }

    // custom model
    if (model == OPENAI_API_COMPATIBLE) {
        m_pGridLayout->itemAtPosition(5, 0)->widget()->show();
        m_pGridLayout->itemAtPosition(5, 1)->widget()->show();
        m_pGridLayout->itemAtPosition(6, 0)->widget()->show();
        m_pGridLayout->itemAtPosition(6, 1)->widget()->show();
        m_pApiKeyLineEdit->setPlaceholderText(tr("Optional"));
    } else if (model == OPENROUTER_MODEL) {
        m_pGridLayout->itemAtPosition(5, 0)->widget()->show();
        m_pGridLayout->itemAtPosition(5, 1)->widget()->show();
        m_pGridLayout->itemAtPosition(6, 0)->widget()->hide();
        m_pGridLayout->itemAtPosition(6, 1)->widget()->hide();
        m_pGridLayout->itemAtPosition(8, 0)->widget()->show();
        m_pGridLayout->itemAtPosition(8, 1)->widget()->show();
        m_pCustomModelName->setPlaceholderText(tr("Required, please input"));
        m_pApiKeyLineEdit->setPlaceholderText(tr("Required, please input"));
        getOpenRouterModelList();
    }else {
        m_pGridLayout->itemAtPosition(5, 0)->widget()->hide();
        m_pGridLayout->itemAtPosition(5, 1)->widget()->hide();
        m_pGridLayout->itemAtPosition(6, 0)->widget()->hide();
        m_pGridLayout->itemAtPosition(6, 1)->widget()->hide();
        m_pApiKeyLineEdit->setPlaceholderText(tr("Required, please input"));
    }

    if (m_lastModelIndex >= 0) {
        if (LLMServerProxy::llmManufacturer(m_modelMap.value(index)) !=
                LLMServerProxy::llmManufacturer(m_modelMap.value(m_lastModelIndex))) {
            m_pAppIdLineEdit->clear();
            m_pApiKeyLineEdit->clear();
            m_pApiSecretLineEdit->clear();
            m_pAddressLineEdit->clear();
        }

        m_pCustomModelName->clear();
        m_pCustomModelUrl->clear();
    }

    if (model == LLMChatModel::WXQF_ERNIE_Bot || model == LLMChatModel::SPARKDESK) {
        m_pGridLayout->itemAtPosition(7, 0)->widget()->show();
        m_pGridLayout->itemAtPosition(7, 1)->widget()->show();
    } else {
        m_pGridLayout->itemAtPosition(7, 0)->widget()->hide();
        m_pGridLayout->itemAtPosition(7, 1)->widget()->hide();
    }

    onUpdateSubmitButtonStatus();
    if (!isHidden()) {
        m_pGridLayout->activate();
        adjustSize();
    }

    m_lastModelIndex = index;
}

void AddModelDialog::onUpdateSubmitButtonStatus()
{
    LLMChatModel model = m_modelMap.value(m_pModelComboBox->currentIndex());
    bool disable = true;

    if (!m_threeKeyComboxIndex.contains(model)) {
        disable = m_pApiKeyLineEdit->text().isEmpty() || m_pNameLineEdit->text().isEmpty() || m_pNameLineEdit->isAlert();
    } else {
        disable = m_pAppIdLineEdit->text().isEmpty() || m_pApiKeyLineEdit->text().isEmpty() || m_pApiSecretLineEdit->text().isEmpty()
                || m_pNameLineEdit->text().isEmpty() || m_pNameLineEdit->isAlert();
    }

    if (model == LLMChatModel::OPENAI_API_COMPATIBLE) {
        disable = m_pNameLineEdit->text().isEmpty() || m_pCustomModelUrl->text().isEmpty();
    }
    else if(model == LLMChatModel::OPENROUTER_MODEL) {
        disable = m_pNameLineEdit->text().isEmpty() || m_pCustomModelName->text().isEmpty() || m_pApiKeyLineEdit->text().isEmpty();
    }


    m_pSubmitButton->setDisabled(disable);
}

bool AddModelDialog::event(QEvent *event)
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

bool AddModelDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_pNameLineEdit->lineEdit() && event->type() == QEvent::FocusOut) {
        isNameDuplicate(DbWrapper::localDbWrapper().queryLlmList(true));
    }
    return DAbstractDialog::eventFilter(watched, event);
}

bool AddModelDialog::isNameDuplicate(const QList<LLMServerProxy> &llmList) const
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

void AddModelDialog::getOpenRouterModelList()
{
    qCInfo(logAIGUI) << "Requesting OpenRouter model list.";
    if(m_openrouterModelIDLst.isEmpty() || m_openrouterModelNameLst.isEmpty())
    {
        QNetworkAccessManager manager;

        QNetworkRequest request;
        request.setUrl(QUrl("https://openrouter.ai/api/v1/models"));

        QNetworkReply *reply = manager.get(request);

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            QString response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8());

            if (!jsonDoc.isNull()) {
                if (jsonDoc.isNull()) {
                    qCCritical(logAIGUI) << "Failed to parse JSON from OpenRouter response.";
                    return;
                }

                QJsonObject jsonObj = jsonDoc.object();
                QJsonArray dataArray = jsonObj["data"].toArray();

                for (const QJsonValue &value : dataArray) {
                    QJsonObject obj = value.toObject();
                    QString id = obj["id"].toString();
                    QString name = obj["name"].toString();
                    qCDebug(logAIGUI) << "OpenRouter Model ID:" << id << ", Name:" << name;

                    m_openrouterModelIDLst.append(id);
                    m_openrouterModelNameLst.append(name);
                }
            } else {
                qCCritical(logAIGUI) << "Failed to parse JSON response from OpenRouter.";
            }
        } else {
            qCCritical(logAIGUI) << "Network request to OpenRouter failed:" << reply->errorString();
        }
        reply->deleteLater();
    }

    for(const QString modelName : m_openrouterModelNameLst) {
        m_modelLstComboBox->addItem(modelName);
    }
    qCInfo(logAIGUI) << "OpenRouter model list loaded. Count:" << m_openrouterModelNameLst.size();
}

void AddModelDialog::onCompactModeChanged()
{
    int height = GUtils::isCompactMode() ? 24 : 36;
    for (auto w : m_widgetSet) {
        if (w) w->setFixedHeight(height);
    }

    m_pWidget->setMinimumHeight(GUtils::isCompactMode() ? 190 : 250);
    //m_pSpinner->move(243, GUtils::isCompactMode() ? 120 : 156);
    this->adjustSize();
}

void AddModelDialog::onUpdateSystemFont(const QFont &)
{
    updateProxyLabel();
    this->adjustSize();
}

void AddModelDialog::onOpenRouterIndexChanged(int index)
{
    if (index > 0 && !m_openrouterModelIDLst.isEmpty()){
        QString modelID = m_openrouterModelIDLst.value(index - 1);
        m_pCustomModelName->setText(modelID);
    }
    else if(index == 0) {
        m_pCustomModelName->setText("");
    }
}

void AddModelDialog::updateProxyLabel()
{
    const QColor &color = DPaletteHelper::instance()->palette(m_pProxyLabel).color(DPalette::Normal, DPalette::Highlight);
    int width = static_cast<int>((m_pProxyLabel->font().pixelSize() - 4) * QGuiApplication::primaryScreen()->devicePixelRatio());
    m_pProxyLabel->setText(tr("For proxy settings, please go to system proxy settings")
                           + QString("<a href=\"javascript:void(0)\" style=\"color:%1; text-decoration: none;\"> %2<img src=\"%3\" width=\"%4\" height=\"%4\"></a>")
                           .arg(color.name())
                           .arg(tr("Go to settings"))
                           .arg(GUtils::generateImage(m_pProxyLabel, color, QSize(width, width), QStyle::PE_IndicatorArrowRight))
                           .arg(m_pProxyLabel->font().pixelSize() - 4)
                          );
}

void AddModelDialog::resetDialog()
{
    m_pModelComboBox->setCurrentIndex(0);
    m_pAppIdLineEdit->clear();
    m_pApiKeyLineEdit->clear();
    m_pApiSecretLineEdit->clear();
    m_pNameLineEdit->clear();
    m_pAddressLineEdit->clear();
    m_pRaidersButton->resetUrl();
    m_pCustomModelName->clear();
    m_pCustomModelUrl->clear();
    m_modelLstComboBox->setCurrentIndex(0);
}
