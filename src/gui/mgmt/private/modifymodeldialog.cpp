#include "modifymodeldialog.h"
#include "dbwrapper.h"

#include <QApplication>

ModifyModelDialog::ModifyModelDialog(const LLMServerProxy &data, DWidget *parent)
    : DAbstractDialog(parent)
{
    setFixedSize(528, 410);

    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);
    DFontSizeManager::instance()->bind(titleBar, DFontSizeManager::T5, QFont::DemiBold);
    titleBar->setTitle(data.name);
    auto labels = titleBar->findChildren<DLabel *>();
    for (auto label : labels) {
        if (label->accessibleName() == "DTitlebarCenterArea") {
            label->setTextFormat(Qt::PlainText);
            break;
        }
    }

    DLabel *modelLabel = new DLabel(tr("LLM"));
    modelLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(modelLabel, DFontSizeManager::T6, QFont::Medium);
    modelLabel->setFixedHeight(36);
    modelLabel->setToolTip(modelLabel->text());

    m_pModelLabel = new DLabel;
    m_pModelLabel->setFixedSize(381, 36);
    DFontSizeManager::instance()->bind(m_pModelLabel, DFontSizeManager::T6, QFont::Medium);

    DLabel *appIdLabel = new DLabel(tr("APPID"));
    appIdLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(appIdLabel, DFontSizeManager::T6, QFont::Medium);
    appIdLabel->setFixedHeight(36);
    appIdLabel->setToolTip(appIdLabel->text());

    m_pAppIdLabel = new DLabel;
    m_pAppIdLabel->setFixedSize(381, 36);
    DFontSizeManager::instance()->bind(m_pAppIdLabel, DFontSizeManager::T6, QFont::Medium);

    DLabel *apiKeyLabel = new DLabel(tr("APIKey"));
    apiKeyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(apiKeyLabel, DFontSizeManager::T6, QFont::Medium);
    apiKeyLabel->setFixedHeight(36);
    apiKeyLabel->setToolTip(apiKeyLabel->text());

    m_pApiKeyLabel = new DLabel;
    m_pApiKeyLabel->setFixedSize(381, 36);
    DFontSizeManager::instance()->bind(m_pApiKeyLabel, DFontSizeManager::T6, QFont::Medium);

    DLabel *apiSecretLabel = new DLabel(tr("APISecret"));
    apiSecretLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(apiSecretLabel, DFontSizeManager::T6, QFont::Medium);
    apiSecretLabel->setFixedHeight(36);
    apiSecretLabel->setToolTip(apiSecretLabel->text());

    m_pApiSecretLabel = new DLabel;
    m_pApiSecretLabel->setFixedSize(381, 36);
    DFontSizeManager::instance()->bind(m_pApiSecretLabel, DFontSizeManager::T6, QFont::Medium);

    DLabel *accountNameLabel = new DLabel(tr("Account"));
    accountNameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(accountNameLabel, DFontSizeManager::T6, QFont::Medium);
    accountNameLabel->setFixedHeight(36);
    accountNameLabel->setToolTip(accountNameLabel->text());

    DLabel *apiModelLabel = new DLabel(tr("Model Name"));
    apiModelLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(apiModelLabel, DFontSizeManager::T6, QFont::Medium);
    apiModelLabel->setFixedHeight(36);
    apiModelLabel->setToolTip(apiModelLabel->text());

    m_pApiModelLabel = new DLabel;
    m_pApiModelLabel->setFixedSize(381, 36);
    DFontSizeManager::instance()->bind(m_pApiModelLabel, DFontSizeManager::T6, QFont::Medium);

    DLabel *apiUrlLabel = new DLabel(tr("API Address"));
    apiUrlLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(apiUrlLabel, DFontSizeManager::T6, QFont::Medium);
    apiUrlLabel->setFixedHeight(36);
    apiUrlLabel->setToolTip(apiUrlLabel->text());

    m_pApiUrlLabel = new DLabel;
    m_pApiUrlLabel->setFixedSize(381, 36);
    DFontSizeManager::instance()->bind(m_pApiUrlLabel, DFontSizeManager::T6, QFont::Medium);

    m_pNameLineEdit = new DLineEdit();
    m_pNameLineEdit->setFixedSize(381, 36);
    m_pNameLineEdit->lineEdit()->setMaxLength(21);
    m_pNameLineEdit->setPlaceholderText(tr("Required, to distinguish multiple models"));
    m_pNameLineEdit->lineEdit()->installEventFilter(this);
    m_pNameLineEdit->lineEdit()->setProperty("_d_dtk_lineedit_opacity", false);

    DLabel *requestAddressLabel = new DLabel(tr("Domain"));
    requestAddressLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(requestAddressLabel, DFontSizeManager::T6, QFont::Medium);
    requestAddressLabel->setFixedHeight(36);
    requestAddressLabel->setToolTip(requestAddressLabel->text());
    m_pRequestAddressLabel = new DLabel;
    m_pRequestAddressLabel->setFixedWidth(381);
    m_pRequestAddressLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_pRequestAddressLabel, DFontSizeManager::T6, QFont::Medium);

    m_pContextLayout = new QGridLayout();
    m_pContextLayout->setContentsMargins(20, 6, 20, 0);
    m_pContextLayout->setHorizontalSpacing(20);
    m_pContextLayout->setVerticalSpacing(10);
    m_pContextLayout->addWidget(accountNameLabel, 0, 0);
    m_pContextLayout->addWidget(m_pNameLineEdit, 0, 1);
    m_pContextLayout->addWidget(modelLabel, 1, 0);
    m_pContextLayout->addWidget(m_pModelLabel, 1, 1);
    m_pContextLayout->addWidget(appIdLabel, 2, 0);
    m_pContextLayout->addWidget(m_pAppIdLabel, 2, 1);
    m_pContextLayout->addWidget(apiKeyLabel, 3, 0);
    m_pContextLayout->addWidget(m_pApiKeyLabel, 3, 1);
    m_pContextLayout->addWidget(apiSecretLabel, 4, 0);
    m_pContextLayout->addWidget(m_pApiSecretLabel, 4, 1);
    m_pContextLayout->addWidget(apiModelLabel, 5, 0);
    m_pContextLayout->addWidget(m_pApiModelLabel, 5, 1);
    m_pContextLayout->addWidget(apiUrlLabel, 6, 0);
    m_pContextLayout->addWidget(m_pApiUrlLabel, 6, 1);
    m_pContextLayout->addWidget(requestAddressLabel, 7, 0);
    m_pContextLayout->addWidget(m_pRequestAddressLabel, 7, 1);
    QHBoxLayout *gridLayout = new QHBoxLayout();
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->addStretch();
    gridLayout->addLayout(m_pContextLayout);
    gridLayout->addStretch();

    DPushButton *cancelButton = new DPushButton(tr("Cancel"));
    cancelButton->setFixedSize(200, 36);
    m_pSubmitButton = new DSuggestButton(tr("Confirm"));
    m_pSubmitButton->setFixedSize(200, 36);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(m_pSubmitButton);
    buttonLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 10);
    mainLayout->addWidget(titleBar);
    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    connect(cancelButton, &DPushButton::clicked, this, [this]() {
        this->reject();
    });

    connect(m_pSubmitButton, &DPushButton::clicked, this, [this]() {
        if (isNameDuplicate(DbWrapper::localDbWrapper().queryLlmList(true))) {
            return;
        }

        this->accept();
    });
    connect(m_pNameLineEdit, &DLineEdit::textChanged, this, &ModifyModelDialog::onNameTextChanged);
    connect(m_pNameLineEdit, &DLineEdit::alertChanged, this, &ModifyModelDialog::onNameAlertChanged);
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));

    m_threeKeyComboxIndex.insert(LLMChatModel::SPARKDESK);
    m_threeKeyComboxIndex.insert(LLMChatModel::SPARKDESK_2);
    m_threeKeyComboxIndex.insert(LLMChatModel::SPARKDESK_3);
    m_threeKeyComboxIndex.insert(LLMChatModel::WXQF_ERNIE_Bot);
    m_threeKeyComboxIndex.insert(LLMChatModel::WXQF_ERNIE_Bot_turbo);
    m_threeKeyComboxIndex.insert(LLMChatModel::WXQF_ERNIE_Bot_4);

    setData(data);
    if (data.name.isEmpty())
        titleBar->setTitle(m_pModelLabel->text());
}

void ModifyModelDialog::updateContexts(const LLMChatModel &model)
{
    if (!m_threeKeyComboxIndex.contains(model)) {
        m_pContextLayout->itemAtPosition(2, 0)->widget()->hide();
        m_pContextLayout->itemAtPosition(2, 1)->widget()->hide();
        m_pContextLayout->itemAtPosition(4, 0)->widget()->hide();
        m_pContextLayout->itemAtPosition(4, 1)->widget()->hide();
    } else {
        m_pContextLayout->itemAtPosition(2, 0)->widget()->show();
        m_pContextLayout->itemAtPosition(2, 1)->widget()->show();
        m_pContextLayout->itemAtPosition(4, 0)->widget()->show();
        m_pContextLayout->itemAtPosition(4, 1)->widget()->show();
    }

    // custom model
    if (model == OPENAI_API_COMPATIBLE) {
        m_pContextLayout->itemAtPosition(5, 0)->widget()->show();
        m_pContextLayout->itemAtPosition(5, 1)->widget()->show();
        m_pContextLayout->itemAtPosition(6, 0)->widget()->show();
        m_pContextLayout->itemAtPosition(6, 1)->widget()->show();
    } else {
        m_pContextLayout->itemAtPosition(5, 0)->widget()->hide();
        m_pContextLayout->itemAtPosition(5, 1)->widget()->hide();
        m_pContextLayout->itemAtPosition(6, 0)->widget()->hide();
        m_pContextLayout->itemAtPosition(6, 1)->widget()->hide();
    }

    if (!m_requestAddress.isEmpty() && m_modelType != FREE_NORMAL && m_modelType != FREE_KOL
            && (model == LLMChatModel::WXQF_ERNIE_Bot || model == LLMChatModel::SPARKDESK)) {
        m_pContextLayout->itemAtPosition(7, 0)->widget()->show();
        m_pContextLayout->itemAtPosition(7, 1)->widget()->show();
    } else {
        m_pContextLayout->itemAtPosition(7, 0)->widget()->hide();
        m_pContextLayout->itemAtPosition(7, 1)->widget()->hide();
    }
}

void ModifyModelDialog::setData(const LLMServerProxy &data)
{
    m_pModelLabel->setText(LLMServerProxy::llmName(data.model, !data.url.isEmpty()));
    if (m_threeKeyComboxIndex.contains(data.model)) {
        m_pAppIdLabel->setText(getDesensitivity(data.account.appId));
        m_pApiSecretLabel->setText(getDesensitivity(data.account.apiSecret));
    }

    if (data.model == LLMChatModel::OPENAI_API_COMPATIBLE) {
        m_pApiModelLabel->setText(data.ext.value(LLM_EXTKEY_VENDOR_MODEL).toString());
        m_pApiUrlLabel->setText(data.ext.value(LLM_EXTKEY_VENDOR_URL).toString());
    }

    if (data.model == LLMChatModel::WXQF_ERNIE_Bot || data.model == LLMChatModel::SPARKDESK) {
        m_pRequestAddressLabel->setText(data.url);
    }

    m_appId = data.account.appId;
    m_apiKey = data.account.apiKey;
    m_apiSecret = data.account.apiSecret;
    m_requestAddress = data.url;
    m_modelType = data.type;

    QFontMetrics fm = m_pAppIdLabel->fontMetrics();
    m_pAppIdLabel->setText(fm.elidedText(getDesensitivity(m_appId), Qt::ElideMiddle, m_pAppIdLabel->width()));
    fm = m_pApiKeyLabel->fontMetrics();
    m_pApiKeyLabel->setText(fm.elidedText(getDesensitivity(m_apiKey), Qt::ElideMiddle, m_pApiKeyLabel->width()));
    fm = m_pApiSecretLabel->fontMetrics();
    m_pApiSecretLabel->setText(fm.elidedText(getDesensitivity(m_apiSecret), Qt::ElideMiddle, m_pApiSecretLabel->width()));

    m_name = data.name;
    m_pNameLineEdit->setText(data.name);
    updateContexts(data.model);
    m_pSubmitButton->setDisabled(m_pNameLineEdit->text().isEmpty());
}

QString ModifyModelDialog::getModelName()
{
    return m_pNameLineEdit->text();
}

QString ModifyModelDialog::getDesensitivity(const QString &input)
{
    if (input.length() < 5) {
        return input;
    } else if (input.length() <= 8) {
        QString prefix = input.left(2);
        QString suffix = input.right(3);

        return prefix + QString(input.length() - 5, '*') + suffix;
    } else {
        QString prefix = input.left(4);
        QString suffix = input.right(5);

        return prefix + QString(input.length() - 9, '*') + suffix;
    }
}

void ModifyModelDialog::onNameTextChanged(const QString &str)
{
    m_pNameLineEdit->setAlert(false);
    m_pNameLineEdit->hideAlertMessage();

    if (str.length() > 20) {
        m_pNameLineEdit->blockSignals(true);
        m_pNameLineEdit->showAlertMessage(tr("No more than 20 characters"));
        m_pNameLineEdit->setText(str.left(20));
        m_pNameLineEdit->blockSignals(false);
    }

    m_pSubmitButton->setDisabled(m_pNameLineEdit->text().isEmpty() || m_pNameLineEdit->isAlert());
}

void ModifyModelDialog::onNameAlertChanged(bool alert)
{
    if (alert)
        m_pSubmitButton->setDisabled(true);
}

void ModifyModelDialog::onUpdateSystemFont(const QFont &)
{
    QFontMetrics fm = m_pAppIdLabel->fontMetrics();
    m_pAppIdLabel->setText(fm.elidedText(getDesensitivity(m_appId), Qt::ElideMiddle, m_pAppIdLabel->width()));
    fm = m_pApiKeyLabel->fontMetrics();
    m_pApiKeyLabel->setText(fm.elidedText(getDesensitivity(m_apiKey), Qt::ElideMiddle, m_pApiKeyLabel->width()));
    fm = m_pApiSecretLabel->fontMetrics();
    m_pApiSecretLabel->setText(fm.elidedText(getDesensitivity(m_apiSecret), Qt::ElideMiddle, m_pApiSecretLabel->width()));
}

bool ModifyModelDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_pNameLineEdit->lineEdit() && event->type() == QEvent::FocusOut) {
        isNameDuplicate(DbWrapper::localDbWrapper().queryLlmList(true));
    }
    return DAbstractDialog::eventFilter(watched, event);
}

bool ModifyModelDialog::isNameDuplicate(const QList<LLMServerProxy> &llmList) const
{
    for (auto llm : llmList) {
        if (llm.name.trimmed() == m_pNameLineEdit->text().trimmed() && m_name != m_pNameLineEdit->text().trimmed()) {
            m_pNameLineEdit->setAlert(true);
            m_pNameLineEdit->showAlertMessage(tr("The account name already exists, please change it"), -1);
            return true;
        }
    }
    return false;
}
