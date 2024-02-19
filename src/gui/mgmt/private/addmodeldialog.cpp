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

#include <QUuid>

#include <DMessageManager>
#include <DApplicationHelper>

AddModelDialog::AddModelDialog(DWidget *parent)
    : DAbstractDialog(parent)
{
    EWndManager()->registeWindow(this);

    initUI();
    initConnect();

    setModal(true);
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

    m_pModelComboBox = new DComboBox();
#ifndef QT_DEBUG
    if (QLocale::Chinese != QLocale::system().language() || QLocale::SimplifiedChineseScript != QLocale::system().script()) {
        m_modelMap.insert(m_modelMap.size(), LLMChatModel::CHATGPT_3_5_16K);
        m_modelMap.insert(m_modelMap.size(), LLMChatModel::CHATGPT_4);
    }
#else
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::CHATGPT_3_5_16K);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::CHATGPT_4);
#endif
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::WXQF_ERNIE_Bot);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::WXQF_ERNIE_Bot_turbo);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::WXQF_ERNIE_Bot_4);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::SPARKDESK);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::SPARKDESK_2);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::SPARKDESK_3);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::GPT360_S2_V9);
    m_modelMap.insert(m_modelMap.size(), LLMChatModel::ChatZHIPUGLM_PRO);
    for (int i = 0; i < m_modelMap.size(); i++) {
        m_pModelComboBox->addItem(LLMServerProxy::llmName(m_modelMap.value(i)));
    }
    m_pModelComboBox->setFixedWidth(381);
    m_threeKeyComboxIndex.insert(WXQF_ERNIE_Bot);
    m_threeKeyComboxIndex.insert(WXQF_ERNIE_Bot_turbo);
    m_threeKeyComboxIndex.insert(WXQF_ERNIE_Bot_4);
    m_threeKeyComboxIndex.insert(SPARKDESK);
    m_threeKeyComboxIndex.insert(SPARKDESK_2);
    m_threeKeyComboxIndex.insert(SPARKDESK_3);

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
    warningLayout->addStretch();
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
    proxyLayout->setContentsMargins(20, 0, 20, 0);
    proxyLayout->setMargin(0);
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
    buttonLayout->setMargin(0);
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
    m_pSpinner->move(243, GUtils::isCompactMode() ? 120 : 156);

    onComboBoxIndexChanged(m_pModelComboBox->currentIndex());

    m_widgetSet.insert(modelLabel);
    m_widgetSet.insert(appIdLabel);
    m_widgetSet.insert(apiKeyLabel);
    m_widgetSet.insert(apiSecretLabel);
    m_widgetSet.insert(accountNameLabel);

    m_widgetSet.insert(m_pModelComboBox);
    m_widgetSet.insert(m_pAppIdLineEdit);
    m_widgetSet.insert(m_pApiKeyLineEdit);
    m_widgetSet.insert(m_pApiSecretLineEdit);
    m_widgetSet.insert(m_pNameLineEdit);
    m_widgetSet.insert(m_pCancelButton);
    m_widgetSet.insert(m_pSubmitButton);

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
    connect(m_pNameLineEdit, &DLineEdit::textChanged, this, &AddModelDialog::onNameTextChanged);
    connect(m_pNameLineEdit, &DLineEdit::alertChanged, this, &AddModelDialog::onNameAlertChanged);
    GUtils::connection(DGuiApplicationHelper::instance(), "sizeModeChanged(DGuiApplicationHelper::SizeMode)", this, "onCompactModeChanged()");
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));
    connect(m_pProxyLabel, &DLabel::linkActivated, this, []() {
        DbusControlCenterRequest dbus;
        dbus.showPage("network", "System Proxy");
    });
}

void AddModelDialog::setAllWidgetEnabled(bool enable)
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

void AddModelDialog::onNameTextChanged(const QString &str)
{
    m_pNameLineEdit->setAlert(false);
    m_pNameLineEdit->hideAlertMessage();

    if (str.length() > 20) {
        m_pNameLineEdit->blockSignals(true);
        m_pNameLineEdit->showAlertMessage(tr("No more than 20 characters"));
        m_pNameLineEdit->setText(str.left(20));
        m_pNameLineEdit->blockSignals(false);
    }
    onUpdateSubmitButtonStatus();
}

void AddModelDialog::onNameAlertChanged(bool alert)
{
    if (alert)
        m_pSubmitButton->setDisabled(true);
}

void AddModelDialog::onSubmitButtonClicked()
{
    auto llmList = DbWrapper::localDbWrapper().queryLlmList(true);
    if (isNameDuplicate(llmList)) {
        return;
    }

    LLMChatModel model = m_modelMap.value(m_pModelComboBox->currentIndex());
    QString apiKey = m_pApiKeyLineEdit->text().trimmed();
    QString appId = m_pAppIdLineEdit->text().trimmed();
    QString apiSecret = m_pApiSecretLineEdit->text().trimmed();

    bool exist = false;
    for (auto llm : llmList) {
        if (llm.model != model)
            continue;

        // gpt 360 zhipu，只判断key重复
        if ((llm.model == LLMChatModel::CHATGPT_3_5_16K || llm.model == LLMChatModel::CHATGPT_4
                || llm.model == LLMChatModel::GPT360_S2_V9
                || llm.model == LLMChatModel::ChatZHIPUGLM_PRO)
                && llm.account.apiKey == apiKey) {
            exist = true;
            break;
        }

        // 讯飞、百度，判断key id secret重复
        if (m_threeKeyComboxIndex.contains(llm.model)
                && llm.account.appId == appId && llm.account.apiKey == apiKey && llm.account.apiSecret == apiSecret) {
            exist = true;
            break;
        }
    }

    if (exist) {
        CommonFailDialog dlg(this);
        dlg.setFailMsg(tr("This LLM already exists, please do not add it again."));
        dlg.exec();
        return;
    }

    //验证KOL账号
    if (m_threeKeyComboxIndex.contains(model)) {
        //解密成功才可能是kol账号，不成功走直接模型服务器验证逻辑
        UosAccountEncoder encoder;
        if (!std::get<0>(encoder.decrypt(appId)).isEmpty()
                && !std::get<0>(encoder.decrypt(apiKey)).isEmpty()
                && !std::get<0>(encoder.decrypt(apiSecret)).isEmpty()) {
            UosFreeAccount freeAccount;
            freeAccount.appid = appId;
            freeAccount.appkey = apiKey;
            freeAccount.appsecret = apiSecret;
            int status;

            setAllWidgetEnabled(false);
            QNetworkReply::NetworkError error = UosFreeAccounts::instance().getFreeAccount(ModelType::FREE_KOL, freeAccount, status);
            setAllWidgetEnabled(true);

            if (QNetworkReply::NoError == error) {
                LLMChatModel m = static_cast<LLMChatModel>(freeAccount.llmModel);
                bool typeError = false; //模型错误，百度两种模型账号可以混用，但是百度和讯飞不能混用
                // 选择正确的时候
                if (m != model) {
                    if (m == LLMChatModel::WXQF_ERNIE_Bot || m == LLMChatModel::WXQF_ERNIE_Bot_turbo || m == LLMChatModel::WXQF_ERNIE_Bot_4) {
                        m = model;
                    } else {
                        typeError = true;
                    }
                }

                if (!typeError) {
                    //是kol账号添加到界面去
                    LLMServerProxy llm;
                    llm.type = ModelType::FREE_KOL;
                    llm.name = m_pNameLineEdit->text();
                    llm.id = freeAccount.appkey + "_" + model;
                    llm.model = m;
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
                        CommonFailDialog dlg(this);
                        dlg.setFailMsg(tr("Save failed, please try again later"));
                        dlg.exec();
                        return;
                    }

                    ServerWrapper::instance()->updateLLMAccount();
                    this->accept();
                    return;
                }
            } else if (1 != status && 3 != status) {
                //其他错误，但不是1和3，说明是网络或者服务器异常了
                CommonFailDialog dlg(this);
                dlg.setFailMsg(tr("Network exception, please try again later."));
                dlg.exec();
                return;
            }
            // 其他场景，说明不是kol账号，走正常添加流程
        }
    }

    LLMServerProxy serverProxy;
    serverProxy.name = m_pNameLineEdit->text();
    serverProxy.id = QUuid::createUuid().toString(QUuid::Id128);
    serverProxy.model = model;

    AccountProxy accountProxy;
    SocketProxy socketProxy;
    socketProxy.socketProxyType = SocketProxyType::SYSTEM_PROXY;
    accountProxy.socketProxy = socketProxy;
    accountProxy.appId = appId;
    accountProxy.apiKey = apiKey;
    accountProxy.apiSecret = apiSecret;
    serverProxy.account = accountProxy;
    serverProxy.type = ModelType::USER; //设置默认类型，防止未初始化就使用
    m_data = serverProxy;

    setAllWidgetEnabled(false);
    QPair<int, QString> pair = ServerWrapper::instance()->verify(serverProxy);
    setAllWidgetEnabled(true);

    if (0 != pair.first) {
        CommonFailDialog dlg(this);
        dlg.setFailMsg(pair.second);
        dlg.exec();
        return;
    }

    if (!DbWrapper::localDbWrapper().appendLlm(serverProxy)) {
        CommonFailDialog dlg(this);
        dlg.setFailMsg(tr("Save failed, please try again later"));
        dlg.exec();
        return;
    }

    ServerWrapper::instance()->updateLLMAccount();
    this->accept();
}

LLMServerProxy AddModelDialog::getModelData()
{
    return m_data;
}

void AddModelDialog::onComboBoxIndexChanged(int index)
{
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

    m_pAppIdLineEdit->clear();
    m_pApiKeyLineEdit->clear();
    m_pApiSecretLineEdit->clear();
    m_pNameLineEdit->clear();

    onUpdateSubmitButtonStatus();
    if (!isHidden()) adjustSize();
}

void AddModelDialog::onUpdateSubmitButtonStatus()
{
    LLMChatModel model = m_modelMap.value(m_pModelComboBox->currentIndex()) ;
    if (!m_threeKeyComboxIndex.contains(model)) {
        m_pSubmitButton->setDisabled(m_pApiKeyLineEdit->text().isEmpty() || m_pNameLineEdit->text().isEmpty() || m_pNameLineEdit->isAlert());
    } else {
        m_pSubmitButton->setDisabled(m_pAppIdLineEdit->text().isEmpty() || m_pApiKeyLineEdit->text().isEmpty() || m_pApiSecretLineEdit->text().isEmpty()
                                     || m_pNameLineEdit->text().isEmpty() || m_pNameLineEdit->isAlert());
    }
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

void AddModelDialog::onCompactModeChanged()
{
    int height = GUtils::isCompactMode() ? 24 : 36;
    for (auto w : m_widgetSet) {
        if (w) w->setFixedHeight(height);
    }

    m_pWidget->setMinimumHeight(GUtils::isCompactMode() ? 190 : 250);
    m_pSpinner->move(243, GUtils::isCompactMode() ? 120 : 156);
    this->adjustSize();
}

void AddModelDialog::onUpdateSystemFont(const QFont &)
{
    updateProxyLabel();
    this->adjustSize();
}

void AddModelDialog::updateProxyLabel()
{
    const QColor &color = DApplicationHelper::instance()->palette(m_pProxyLabel).color(DPalette::Normal, DPalette::Highlight);
    m_pProxyLabel->setText(tr("For proxy settings, please go to system proxy settings")
                           + QString("<a href=\"javascript:void(0)\" style=\"color:%1; text-decoration: none;\"> %2<img src=\"%3\"></a>")
                           .arg(color.name())
                           .arg(tr("Go to settings"))
                           .arg(GUtils::generateImage(m_pProxyLabel, color, QSize(m_pProxyLabel->font().pixelSize() - 4, m_pProxyLabel->font().pixelSize() - 4), QStyle::PE_IndicatorArrowRight))
                          );
}

void AddModelDialog::resetDialog()
{
    m_pModelComboBox->setCurrentIndex(0);
    m_pAppIdLineEdit->clear();
    m_pApiKeyLineEdit->clear();
    m_pApiSecretLineEdit->clear();
    m_pNameLineEdit->clear();
    m_pRaidersButton->resetUrl();
}
