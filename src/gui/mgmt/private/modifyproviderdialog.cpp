#include "modifyproviderdialog.h"
#include "builtinprovider.h"
#include "global_key_define.h"
#include "appdatabase.h"
#include "modelvendor.h"
#include "model/modelvalidator.h"
#include "gui/gutils.h"
#include "database/modelstable.h"
#include "database/providertable.h"
#include "utils/esystemcontext.h"
#include "oscontrol/deepincontrolcenter.h"
#include "commonfaildialog.h"

#include <DPaletteHelper>
#include <DHorizontalLine>
#include <DLineEdit>
#include <DFontSizeManager>
#include <DScrollArea>
#include <DDialog>

#include <QMessageBox>
#include <QUuid>
#include <QLoggingCategory>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMap>
#include <QStringList>
#include <QApplication>
#include <QScreen>
#include <QPaintEvent>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)
DWIDGET_USE_NAMESPACE

using namespace uos_ai;

ModifyProviderDialog::ModifyProviderDialog(bool priavte, QWidget *parent)
    : DAbstractDialog(parent)
    , m_editMode(false)
    , m_onlyPrivate(priavte)
{
    // 支持的厂商列表
    if (m_onlyPrivate) {
        m_providerList = QStringList{ STR_KEY_PRIVATE_NET };
    } else {
        m_providerList = QStringList{STR_KEY_DEEPSEEK,
                                           STR_KEY_MINIMAX,
                                           STR_KEY_VOLCENGINE,
                                           STR_KEY_MOONSHOT,
                                           STR_KEY_BIGMODEL,
                                           STR_KEY_BAILIAN,
                                           STR_KEY_OPENAI_COMPATIBLE};
    }

    setModal(true);
    initUI();
    initConnect();
    initEmpty();

    if (ESystemContext::isWayland()) {
        //wayland环境模态dialog需要手动置顶
        setWindowFlags(windowFlags() | Qt::Dialog |  Qt::WindowStaysOnTopHint);
    }
}

ModifyProviderDialog::ModifyProviderDialog(const ProviderAccount &provider, const QList<ModelAccountPtr> &models,
                                           bool priavte, QWidget *parent)
    : DAbstractDialog(parent)
    , m_editMode(true)
    , m_onlyPrivate(priavte)
    , m_provider(provider)
    , m_models(models)
{
    m_providerList = QStringList{ provider.provider };

    setModal(true);
    initUI();
    initConnect();
    initData();

    if (ESystemContext::isWayland()) {
        //wayland环境模态dialog需要手动置顶
        setWindowFlags(windowFlags() | Qt::Dialog |  Qt::WindowStaysOnTopHint);
    }
}

void ModifyProviderDialog::updateProxyLabel()
{
    const QColor &color = DPaletteHelper::instance()->palette(m_proxyLabel).color(DPalette::Normal, DPalette::Highlight);
    int width = static_cast<int>((m_proxyLabel->font().pixelSize() - 4) * QGuiApplication::primaryScreen()->devicePixelRatio());
    m_proxyLabel->setText(tr("For proxy settings, please go to system proxy settings")
                           + QString("<a href=\"javascript:void(0)\" style=\"color:%1; text-decoration: none;\"> %2<img src=\"%3\" width=\"%4\" height=\"%4\"></a>")
                           .arg(color.name())
                           .arg(tr("Go to settings"))
                           .arg(GUtils::generateImage(m_proxyLabel, color, QSize(width, width), QStyle::PE_IndicatorArrowRight))
                           .arg(m_proxyLabel->font().pixelSize() - 4)
                          );
    update();
}

void ModifyProviderDialog::resetEdit()
{
    m_editButton->setProperty("editMode", false);
    m_editButton->setText(tr("Delete"));
    m_addButton->setDisabled(false);

    for (CustomModelGroup *model : m_customContent->findChildren<CustomModelGroup *>()) {
        model->setEditMode(false);
    }
}

void ModifyProviderDialog::initUI()
{
    const int editWidth = 381;
    const int widgetHeight = 36;

    setFixedWidth(518);

    auto createVLay = [](QWidget *w) {
        auto wrapper = new DWidget;
        auto lay = new QVBoxLayout;
        lay->setContentsMargins(0, 0, 0, 0);
        lay->addWidget(w);
        lay->addStretch();
        wrapper->setLayout(lay);
        return wrapper;
    };

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(0, 0, 0, 10);

    m_titleBar = new DTitlebar(this);
    m_titleBar->setMenuVisible(false);
    m_titleBar->setBackgroundTransparent(true);
    DFontSizeManager::instance()->bind(m_titleBar, DFontSizeManager::T5, QFont::DemiBold);
    m_titleBar->setTitle(m_editMode ? tr("Edit Model") : tr("Add Model"));
    mainLayout->addWidget(m_titleBar);

    m_gridLayout = new QGridLayout();
    m_gridLayout->setContentsMargins(0, 0, 0, 10); // 下与警告信息留10px空白，
    m_gridLayout->setHorizontalSpacing(10);
    m_gridLayout->setVerticalSpacing(10);
    {
        auto lay = new QHBoxLayout();
        lay->addStretch();
        lay->addLayout(m_gridLayout);
        lay->addStretch();
        lay->setContentsMargins(0, 0, 0, 0);
        mainLayout->addLayout(lay);
    }

    DLabel *accountLabel = new DLabel(tr("Account"));
    accountLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    accountLabel->setToolTip(accountLabel->text());
    accountLabel->setFixedHeight(widgetHeight);
    DFontSizeManager::instance()->bind(accountLabel, DFontSizeManager::T6, QFont::Medium);
    m_gridLayout->addWidget(accountLabel, 0, 0);

    m_accountEdit = new DLineEdit();
    m_accountEdit->setPlaceholderText(tr("Required, to distinguish multiple models"));
    m_accountEdit->setFixedSize(editWidth, widgetHeight);
    m_accountEdit->lineEdit()->setMaxLength(128);
    m_accountEdit->lineEdit()->installEventFilter(this);
    m_accountEdit->lineEdit()->setProperty("_d_dtk_lineedit_opacity", false);
    m_gridLayout->addWidget(m_accountEdit, 0, 1);

    DLabel *providerLabel = new DLabel(tr("Provider"));
    providerLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    providerLabel->setFixedHeight(widgetHeight);
    providerLabel->setToolTip(providerLabel->text());
    DFontSizeManager::instance()->bind(providerLabel, DFontSizeManager::T6, QFont::Medium);
    m_gridLayout->addWidget(providerLabel, 1, 0);

    m_providerComboBox = new DComboBox();
    m_accountEdit->setFixedSize(editWidth, widgetHeight);
    m_gridLayout->addWidget(m_providerComboBox, 1, 1);
    {

        for (const QString &id : m_providerList) {
            auto info = BuiltinProvider::instance()->queryProvider(id);
            m_providerComboBox->addItem(info.name, info.id);
        }
    }

    DLabel *keyLabel = new DLabel(tr("APIKey"));
    keyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    keyLabel->setToolTip(keyLabel->text());
    keyLabel->setFixedHeight(widgetHeight);
    DFontSizeManager::instance()->bind(keyLabel, DFontSizeManager::T6, QFont::Medium);
    m_gridLayout->addWidget(keyLabel, 2, 0);

    m_apiKeyEdit = new DPasswordEdit(this);
    m_apiKeyEdit->setPlaceholderText(tr("Required, please input"));
    m_apiKeyEdit->setFixedSize(editWidth, widgetHeight);
    m_gridLayout->addWidget(m_apiKeyEdit, 2, 1);

    auto domainLabel = new DLabel(tr("Domain"));
    domainLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    domainLabel->setToolTip(domainLabel->text());
    domainLabel->setFixedHeight(widgetHeight);
    DFontSizeManager::instance()->bind(domainLabel, DFontSizeManager::T6, QFont::Medium);
    m_gridLayout->addWidget(domainLabel, 3, 0);

    m_domainEdit = new DLineEdit();
    m_domainEdit->setPlaceholderText(tr("Required, please input"));
    m_domainEdit->lineEdit()->setMaxLength(512);
    m_domainEdit->setFixedSize(editWidth, widgetHeight);
    m_gridLayout->addWidget(m_domainEdit, 3, 1);

    // 内置模型
    auto switchModelLabel = new DLabel(tr("Enable"));
    switchModelLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    switchModelLabel->setToolTip(switchModelLabel->text());
    switchModelLabel->setFixedHeight(widgetHeight);
    DFontSizeManager::instance()->bind(switchModelLabel, DFontSizeManager::T6, QFont::Medium);
    m_gridLayout->addWidget(createVLay(switchModelLabel), 4, 0);

    {
        auto bgLay = new QVBoxLayout;
        bgLay->setContentsMargins(10, 8, 10, 8);
        bgLay->setSpacing(1);

        auto bg = new DBackgroundGroup(bgLay, this);
        bg->setItemSpacing(10);
        bg->setUseWidgetBackground(false);
        bg->setFixedSize(editWidth, 248);
        m_gridLayout->addWidget(bg, 4, 1);

        auto builinArea = new DScrollArea();
        builinArea->setFrameShape(QFrame::NoFrame);
        builinArea->viewport()->setAutoFillBackground(false);
        builinArea->setContentsMargins(0, 0, 0, 0);
        builinArea->setWidgetResizable(true);
        bgLay->addWidget(builinArea);

        m_bulitinContent = new DWidget;
        builinArea->setWidget(m_bulitinContent);
        // 必须在添加到ScrollArea后设置
        m_bulitinContent->setAutoFillBackground(false);
        auto contentLayout = new QVBoxLayout;
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->setSpacing(10);
        contentLayout->addStretch();
        m_bulitinContent->setLayout(contentLayout);
    }

    // 自定义模型
    DLabel *customLabel = new DLabel(tr("Custom"));
    customLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    customLabel->setToolTip(customLabel->text());
    customLabel->setFixedHeight(widgetHeight);
    DFontSizeManager::instance()->bind(customLabel, DFontSizeManager::T6, QFont::Medium);
    m_gridLayout->addWidget(customLabel, 5, 0);

    // 删除、添加按钮
    {
        auto editWidget = new DWidget;
        auto lay = new QHBoxLayout();
        lay->addStretch();
        lay->setContentsMargins(0, 0, 0, 0);
        editWidget->setLayout(lay);

        m_editButton = new DCommandLinkButton(tr("Delete"), this);
        DFontSizeManager::instance()->bind(m_editButton, DFontSizeManager::T8, QFont::Normal);
        m_editButton->setProperty("editMode", false);
        lay->addWidget(m_editButton);

        m_addButton = new DCommandLinkButton(tr("Add"), this);
        DFontSizeManager::instance()->bind(m_addButton, DFontSizeManager::T8, QFont::Normal);
        lay->addWidget(m_addButton);

        m_gridLayout->addWidget(editWidget, 5, 1);
    }

    auto customArea = new DScrollArea();
    customArea->setFrameShape(QFrame::NoFrame);
    customArea->setAttribute(Qt::WA_TranslucentBackground);
    customArea->setWidgetResizable(true);
    customArea->setFixedSize(editWidth, 173);
    {
        m_customContent = new DWidget;
        m_customContent->setAttribute(Qt::WA_TranslucentBackground);
        auto lay = new QVBoxLayout;
        lay->setContentsMargins(0, 0, 10, 5); // 右10 防止滚动条遮挡按钮。下补5px空隙
        lay->setSpacing(10);
        lay->addStretch();
        m_customContent->setLayout(lay);
        customArea->setWidget(m_customContent);
    }
    m_gridLayout->addWidget(customArea, 6, 1);

    mainLayout->addStretch();

    {
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

        mainLayout->addLayout(warningLayout);
    }
#if 0
    // 攻略区域
    m_raidersButton = new RaidersButton;
    {
        QHBoxLayout *raidersLayout = new QHBoxLayout;
        raidersLayout->setContentsMargins(0, 0, 0, 0);
        raidersLayout->addWidget(m_raidersButton);
        raidersLayout->addStretch();
        mainLayout->addLayout(raidersLayout);
    }
#endif
    // 代理区域
    {
        QHBoxLayout *proxyLayout = new QHBoxLayout();
        proxyLayout->setContentsMargins(0, 0, 0, 0);
        proxyLayout->setSpacing(5);
        m_proxyLabel = new DLabel();
        m_proxyLabel->setFixedWidth(488);
        m_proxyLabel->setAlignment(Qt::AlignCenter);
        DFontSizeManager::instance()->bind(m_proxyLabel, DFontSizeManager::T6, QFont::Medium);
        m_proxyLabel->setWordWrap(true);
        proxyLayout->addWidget(m_proxyLabel);
        mainLayout->addLayout(proxyLayout);
    }

    m_cancelButton = new DPushButton(tr("Cancel"));
    m_cancelButton->setFixedSize(200, widgetHeight);
    m_confirmButton = new DSuggestButton(tr("Confirm"));
    m_confirmButton->setFixedSize(200, widgetHeight);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_confirmButton);
    buttonLayout->addStretch();

    mainLayout->addSpacing(5);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    m_pSpinner = new DSpinner(this);
    m_pSpinner->setFixedSize(32, 32);
    m_pSpinner->hide();

    onFontChanged();
}

void ModifyProviderDialog::initConnect()
{
    connect(m_cancelButton, &DPushButton::clicked, this, &ModifyProviderDialog::onCancelButtonClicked);
    connect(m_confirmButton, &DPushButton::clicked, this, &ModifyProviderDialog::onConfirmButtonClicked);
    connect(m_providerComboBox, qOverload<int>(&DComboBox::currentIndexChanged), this, &ModifyProviderDialog::onProviderChanged);
    connect(m_editButton, &DPushButton::clicked, this, &ModifyProviderDialog::onEdit);
    connect(m_addButton, &DPushButton::clicked, this, &ModifyProviderDialog::onAddModelClicked);
    connect(qApp, &QApplication::fontChanged, this, &ModifyProviderDialog::onFontChanged);

    // 按钮状态
    connect(m_accountEdit, &DPasswordEdit::textChanged, this, &ModifyProviderDialog::switchButtonStatus);
    connect(m_apiKeyEdit, &DPasswordEdit::textChanged, this, &ModifyProviderDialog::switchButtonStatus);
    connect(m_domainEdit, &DPasswordEdit::textChanged, this, &ModifyProviderDialog::switchButtonStatus);

    connect(m_proxyLabel, &DLabel::linkActivated, this, []() {
        DeepinControlCenter dbus;
#ifdef COMPILE_ON_V23
        dbus.ShowPage("network/systemProxy");
#else
        dbus.ShowPage("network", "System Proxy");
#endif
    });
}

void ModifyProviderDialog::initEmpty()
{
    m_providerComboBox->setCurrentIndex(0);
    onProviderChanged(0);
}

void ModifyProviderDialog::initData()
{   
    {
        auto idx = m_providerList.indexOf(m_provider.provider);
        if (m_providerComboBox->currentIndex() != idx)
            m_providerComboBox->setCurrentIndex(idx);
        else
            onProviderChanged(idx);
    }

    bool free = ModelVendor::isUosProvider(m_provider.provider);

    m_providerComboBox->setDisabled(true);

    m_accountEdit->setText(m_provider.name);
    m_accountEdit->setClearButtonEnabled(false);

    m_apiKeyEdit->setText(m_provider.auth.value(STR_KEY_API_KEY).toString());
    m_apiKeyEdit->lineEdit()->setReadOnly(free);
    m_apiKeyEdit->setClearButtonEnabled(false);

    if (isPrivateOrOpenAICompatible(m_provider.provider)) {
        m_domainEdit->setText(m_provider.additional.value(STR_KEY_PROVIDER_HOST).toString());
        m_domainEdit->setClearButtonEnabled(false);

        if (m_models.isEmpty()) {
            createCustomModel();
        } else {
            for (const ModelAccountPtr &model : m_models) {
                auto wid = createCustomModel(false);

                wid->setId(model->id);
                wid->setName(model->model.name);
                wid->setModelId(model->model.modelId);
                wid->setReadOnly(true);
            }
        }
    } else {
        auto allWids = m_bulitinContent->findChildren<BuiltinModelItem *>();
        for (const ModelAccountPtr &model : m_models) {
            for (BuiltinModelItem *wid : allWids) {
                if (model->model.id == wid->id()) {
                    wid->setChecked(true);
                    break;
                }
            }
        }
    }

    switchButtonStatus();
    adjustSize();
}

bool ModifyProviderDialog::isPrivateOrOpenAICompatible(const QString &id) const
{
    return id.compare(STR_KEY_PRIVATE_NET, Qt::CaseInsensitive) == 0 ||
            id.compare(STR_KEY_OPENAI_COMPATIBLE, Qt::CaseInsensitive) == 0;
}

QString ModifyProviderDialog::getDesensitivity(const QString &input)
{
    // 对输入字符串进行脱敏处理
    if (input.length() < 2) {
        return "*";
    } else if (input.length() < 3) {
        // 保留第一个字符，其他字符用星号替换
        return input.left(1) + QString(input.length() - 1, '*');
    } else if (input.length() < 5) {
        // 保留第一个和最后一个字符，其他字符用星号替换 
        return input.left(1) + QString(input.length() - 2, '*') + input.right(1);
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

void ModifyProviderDialog::clearBuiltinModel()
{
    auto all = m_bulitinContent->findChildren<BuiltinModelItem *>();
    for (BuiltinModelItem *model : all) {
        m_bulitinContent->layout()->removeWidget(model);
        model->setParent(nullptr);
        delete model;
    }
}

bool ModifyProviderDialog::confirmEdit()
{
    QString name = m_accountEdit->text().trimmed();
    QString key = m_apiKeyEdit->text().trimmed();
    QString domain;

    if (name.isEmpty() || key.isEmpty()) {
        qCWarning(logAIGUI) << "Modify provider failed, name or key is empty"
                << name << key;
        return false;
    }

    QList<ModelsTable> added;
    QStringList removed;
    if (isPrivateOrOpenAICompatible(m_provider.provider)) {
        domain = m_domainEdit->text().trimmed();
        if (domain.isEmpty()) {
            qCWarning(logAIGUI) << "Modify provider failed, domain is empty";
            return false;
        }

        auto existingModelIds = m_models;
        auto removeExistedModel = [&existingModelIds](const QString &modelId) {
            for (auto it = existingModelIds.begin(); it != existingModelIds.end(); ++it) {
                if ((*it)->id == modelId) {
                    existingModelIds.erase(it);
                    break;
                }
            }
        };

        for (CustomModelGroup *model : m_customContent->findChildren<CustomModelGroup *>()) {
            if (!model->id().isEmpty()) {
                removeExistedModel(model->id());
                continue;
            }
            // new model
            QString name = model->name();
            QString modelId = model->modelId();
            if (name.isEmpty() && modelId.isEmpty()) {
                continue;
            }

            // name 和 modelid 仅有一个为空，需弹窗提示
            if (name.isEmpty() || modelId.isEmpty()) {
                CommonFailDialog dlg(this);
                dlg.setFailMsg(tr("Name and Model ID must be filled in."));
                dlg.exec();
                return false;
            }

            ModelsTable table = ModelsTable::create(
                        GlobalUtil::generateUuid(),
                        m_provider.id,
                        ModelsTable::createModel(ModelArch::MaLanguage, ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall),
                                                 modelId, name)
                        );

            added.append(table);
        }

        for (const auto &model : existingModelIds) {
            removed.append(model->id);
        }
    } else {
        auto existingModelIds = m_models;
        auto removeExistedModel = [&existingModelIds](const QString &id) {
            for (auto it = existingModelIds.begin(); it != existingModelIds.end(); ++it) {
                if ((*it)->model.id == id) {
                    existingModelIds.erase(it);
                    return true;
                }
            }
            return false;
        };

        for (BuiltinModelItem *model : m_bulitinContent->findChildren<BuiltinModelItem *>()) {
            if (model->isChecked()) {
                if (removeExistedModel(model->id())) {
                    continue;
                }

                ModelsTable table = ModelsTable::create(
                            GlobalUtil::generateUuid(),
                            m_provider.id,
                            ModelsTable::createModel(model->id())
                            );

                added.append(table);
            }
        }

        for (const auto &model : existingModelIds) {
            removed.append(model->id);
        }
    }

    // remove
    for (const auto &modelId : removed) {
        AppDatabase::instance()->deleteModel(modelId);
    }

    // add
    for (auto &model : added) {
        model.save(AppDatabase::instance());
    }

    // provider
    bool providerChanged = false;
    {
        if (name != m_provider.name) {
            m_provider.name = name;
            providerChanged = true;
        }

        if (key != m_provider.auth.value(STR_KEY_API_KEY).toString()) {
            m_provider.auth[STR_KEY_API_KEY] = key;
            providerChanged = true;
        }
        
        if (!domain.isEmpty()) {
            if (m_provider.additional.value(STR_KEY_PROVIDER_HOST).toString() != domain) {
                m_provider.additional[STR_KEY_PROVIDER_HOST] = domain;
                providerChanged = true;
            }
        }
        
        if (providerChanged) {          
            ProviderTable table = ProviderTable::create(
                m_provider.id,
                m_provider.name,
                QJsonDocument(QJsonObject::fromVariantHash(m_provider.auth)).toJson(QJsonDocument::Compact),
                m_provider.provider,
                QJsonDocument(QJsonObject::fromVariantHash(m_provider.additional)).toJson(QJsonDocument::Compact)
            );

            table.update(AppDatabase::instance());
        }
    }

    if (!removed.isEmpty() || !added.isEmpty() || providerChanged) {
        ModelVendor::instance()->refresh();
        emit dataChanged();
    }

    return true;
}

bool ModifyProviderDialog::confirmAdd()
{
    QString id = GlobalUtil::generateUuid();
    QString provider = m_providerComboBox->currentData().toString();
    bool isCustom = isPrivateOrOpenAICompatible(provider);

    QList<ModelsTable> added;

    if (isCustom) {
        for (CustomModelGroup *model : m_customContent->findChildren<CustomModelGroup *>()) {
            // new model
            QString name = model->name();
            QString modelId = model->modelId();
            if (name.isEmpty() && modelId.isEmpty()) {
                continue;
            }

            // name 和 modelid 仅有一个为空，需弹窗提示
            if (name.isEmpty() || modelId.isEmpty()) {
                CommonFailDialog dlg(this);
                dlg.setFailMsg(tr("Name and Model ID must be filled in."));
                dlg.exec();
                return false;
            }

            ModelsTable table = ModelsTable::create(
                        GlobalUtil::generateUuid(),
                        id,
                        ModelsTable::createModel(ModelArch::MaLanguage, ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall),
                                                 modelId, name)
                        );

            added.append(table);
        }
    } else {
        for (BuiltinModelItem *model : m_bulitinContent->findChildren<BuiltinModelItem *>()) {
            if (model->isChecked()) {
                ModelsTable table = ModelsTable::create(
                            GlobalUtil::generateUuid(),
                            id,
                            ModelsTable::createModel(model->id())
                            );

                added.append(table);
            }
        }
    }

    {
        QString name = m_accountEdit->text().trimmed();
        QString key = m_apiKeyEdit->text().trimmed();

        if (name.isEmpty() || key.isEmpty() || provider.isEmpty()) {
            qCWarning(logAIGUI) << "Add provider failed, name or key or provider is empty"
                    << name << key << provider;
            return false;
        }

        QJsonObject authObj;
        authObj.insert(STR_KEY_API_KEY, key);

        QJsonObject additionalObj;
        if (isCustom) {
            QString domain = m_domainEdit->text().trimmed();
            if (domain.isEmpty()) {
                qCWarning(logAIGUI) << "Add provider failed, domain is empty" << domain;
                return false;
            }
            additionalObj.insert(STR_KEY_PROVIDER_HOST, domain);
        }

        ProviderTable table = ProviderTable::create(
            id,
            name,
            QJsonDocument(authObj).toJson(QJsonDocument::Compact),
            provider,
            QJsonDocument(additionalObj).toJson(QJsonDocument::Compact)
        );

        if (!table.save(AppDatabase::instance())) {
            qCCritical(logAIGUI) << "Failed to save provder to database." << table.id() << table.name();
            CommonFailDialog dlg(this);
            dlg.setFailMsg(tr("Save failed, please try again later"));
            dlg.exec();
            return false;
        }
    }

    for (auto &model : added) {
        if (!model.save(AppDatabase::instance())) {
            qCCritical(logAIGUI) << "Failed to save provder to database." << model.id() << model.model();
            CommonFailDialog dlg(this);
            dlg.setFailMsg(tr("Save failed, please try again later"));
            dlg.exec();
            return false;
        }
    }

    ModelVendor::instance()->refresh();
    emit dataChanged();
    return true;
}

void ModifyProviderDialog::onCancelButtonClicked()
{
    reject();
}

void ModifyProviderDialog::onConfirmButtonClicked()
{
    bool ok = m_editMode ? confirmEdit() : confirmAdd();

    if (ok)
        accept();
}

void ModifyProviderDialog::onEdit()
{
    bool on = !m_editButton->property("editMode").toBool();
    m_editButton->setText(on ? tr("Done") : tr("Delete"));
    m_addButton->setDisabled(on);

    for (CustomModelGroup *model : m_customContent->findChildren<CustomModelGroup *>()) {
        model->setEditMode(on);
    }

    m_editButton->setProperty("editMode", on);
}

void ModifyProviderDialog::onDeleteModel()
{
    CustomModelGroup *wid = qobject_cast<CustomModelGroup *>(sender());
    if (!wid)
        return;

    m_customContent->layout()->removeWidget(wid);
    wid->setParent(nullptr);
    wid->deleteLater();

    if (m_customContent->findChildren<CustomModelGroup *>().isEmpty()) {
        createCustomModel();

        resetEdit();
        switchButtonStatus();
    }
}

void ModifyProviderDialog::onAddModelClicked()
{
    createCustomModel();
    switchButtonStatus();
}

void ModifyProviderDialog::onTestModelClicked()
{
    ModelAccountPtr account(new ModelAccount);
    QString provider = m_providerComboBox->currentData().toString();
    // 保护
    if (ModelVendor::isUosProvider(provider)) {
        qCWarning(logAIGUI) << "The provider is not supported to test, how do you get here???" << provider;
        return;
    }

    account->account.provider = provider;
    account->account.auth.insert(STR_KEY_API_KEY, m_apiKeyEdit->text());

    bool isCustom = isPrivateOrOpenAICompatible(provider);
    if (isCustom) {
        CustomModelGroup *custom = dynamic_cast<CustomModelGroup *>(sender());
        if (!custom) {
            qCWarning(logAIGUI) << "Check model failed, custom model is null";
            return;
        }
        account->account.additional.insert(STR_KEY_PROVIDER_HOST, m_domainEdit->text());
        account->model.modelId = custom->modelId();
        account->model.arch = ModelArch::MaLanguage;
        account->model.ability = ModelAbilities(ModelAbility::MaText);
    } else {
        BuiltinModelItem *model = qobject_cast<BuiltinModelItem *>(sender());
        if (!model) {
            qCWarning(logAIGUI) << "Check model failed, builtin model is null";
            return;
        }
        account->model = BuiltinProvider::instance()->getModelInfo(model->id());
    }

    setEnableInput(false);

    ModelValidator validator;
    ModelValidator::ModelValidationResult result = validator.validate(account);

    setEnableInput(true);

    if (result.success) {
        DDialog dlg(this);
        dlg.setIcon(QIcon(":assets/images/tips.svg"));
        dlg.setTitle(tr("Test passed")); //测试通过
        dlg.setMessage(tr("The model service is available.")); //模型服务正常
        dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonRecommend);
        dlg.exec();
    } else {
        CommonFailDialog dlg(this);
        dlg.setFailMsg(result.errorMessage);
        dlg.exec();
    }
}

void ModifyProviderDialog::onProviderChanged(int index)
{
    auto id = m_providerComboBox->itemData(index).toString();
    auto info = BuiltinProvider::instance()->queryProvider(id);

    bool custom = isPrivateOrOpenAICompatible(id);
    {
        // domain
        m_gridLayout->itemAtPosition(3, 0)->widget()->setVisible(custom);
        m_gridLayout->itemAtPosition(3, 1)->widget()->setVisible(custom);
        // 内置模型
        m_gridLayout->itemAtPosition(4, 0)->widget()->setVisible(!custom);
        m_gridLayout->itemAtPosition(4, 1)->widget()->setVisible(!custom);
        // 自定义模型
        m_gridLayout->itemAtPosition(5, 0)->widget()->setVisible(custom);
        m_gridLayout->itemAtPosition(5, 1)->widget()->setVisible(custom);
        m_gridLayout->itemAtPosition(6, 1)->widget()->setVisible(custom);
    }

    if (!m_editMode) {
        if (m_customContent->findChildren<CustomModelGroup *>().isEmpty()) {
            createCustomModel();
        }
    }
    
    clearBuiltinModel();

    if (!custom) {
        QList<QString> list;
        bool noCheck = false;
        if (ModelVendor::isUosProvider(id)) {
            list = {UOS_FREE_MODEL_AUTO}; // 免费模型特殊处理，只显示自动模型。
            noCheck = true;
        } else {
            list = info.models;
        }

        for (auto &id : list) {
            auto model = BuiltinProvider::instance()->getModelInfo(id);
            auto wid = createBuiltinModel();
            
            wid->setId(id);
            wid->setName(model.name);
            wid->setHideTest(noCheck);
        }
    }

    resetEdit();
    switchButtonStatus();

    // 有隐藏窗口，需调整两次
    adjustSize();
    adjustSize();
}

void ModifyProviderDialog::switchButtonStatus()
{
    bool hasName = !m_accountEdit->text().trimmed().isEmpty();
    bool hasKey = !m_apiKeyEdit->text().trimmed().isEmpty();
    bool isCustom = isPrivateOrOpenAICompatible(m_providerComboBox->currentData().toString());
    bool hasDomain = true;

    if (isCustom) {
        hasDomain = !m_domainEdit->text().trimmed().isEmpty();
        for (CustomModelGroup *model : m_customContent->findChildren<CustomModelGroup *>()) {
            model->setEnableTest(hasKey && hasDomain && !model->modelId().isEmpty());
        }
    } else {
       for (BuiltinModelItem *model : m_bulitinContent->findChildren<BuiltinModelItem *>()) {
            model->setEnableCheck(hasKey);
        }
    }

    m_confirmButton->setEnabled(hasName && hasKey && hasDomain);
}

void ModifyProviderDialog::onFontChanged()
{
    updateProxyLabel();
    adjustSize();
}

CustomModelGroup *ModifyProviderDialog::createCustomModel(bool pre)
{
    auto wid = new CustomModelGroup;
    connect(wid, &CustomModelGroup::requestRemove, this, &ModifyProviderDialog::onDeleteModel);
    connect(wid, &CustomModelGroup::modelChanged, this, &ModifyProviderDialog::switchButtonStatus);
    connect(wid, &CustomModelGroup::requestCheck, this, &ModifyProviderDialog::onTestModelClicked);
    QVBoxLayout *lay = dynamic_cast<QVBoxLayout *>(m_customContent->layout());

    if (pre) {
        lay->insertWidget(0, wid);
    } else {
        lay->insertWidget(lay->count() - 1, wid);
    }

    return wid;
}

BuiltinModelItem *ModifyProviderDialog::createBuiltinModel(bool pre)
{
    auto wid = new BuiltinModelItem;
    connect(wid, &BuiltinModelItem::requestCheck, this, &ModifyProviderDialog::onTestModelClicked);
    QVBoxLayout *lay = dynamic_cast<QVBoxLayout *>(m_bulitinContent->layout());

    if (pre) {
        lay->insertWidget(0, wid);
    } else {
        lay->insertWidget(lay->count() - 1, wid);
    }

    return wid;
}

void ModifyProviderDialog::setEnableInput(bool enable)
{
    m_accountEdit->setEnabled(enable);
    m_providerComboBox->setEnabled(enable);
    m_apiKeyEdit->setEnabled(enable);
    m_domainEdit->setEnabled(enable);
    m_bulitinContent ->setEnabled(enable);
    m_editButton->setEnabled(enable);
    m_addButton->setEnabled(enable);
    m_customContent->setEnabled(enable);
    m_confirmButton->setEnabled(enable);

    if (enable) {
        m_pSpinner->hide();
        m_pSpinner->stop();
    } else {
        m_pSpinner->move(QRect(QPoint(0, 0), size()).center());
        m_pSpinner->start();
        m_pSpinner->show();
    }
}

CustomModelGroup::CustomModelGroup(QWidget *parent) : DWidget(parent)
{
    setFixedHeight(90);

    auto contentWidget = new DWidget;

    {
        QVBoxLayout *layout = new QVBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(1);
        layout->addWidget(contentWidget);

        auto groupWidget = new DBackgroundGroup(layout, this);
        groupWidget->setContentsMargins(0, 0, 0, 0);
        groupWidget->setItemSpacing(1);
        groupWidget->setUseWidgetBackground(false);

        QVBoxLayout *rootLayout = new QVBoxLayout;
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->addWidget(groupWidget);
        setLayout(rootLayout);
    }

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);
    contentWidget->setLayout(layout);

    m_idEdit = new DLineEdit();
    m_idEdit->setPlaceholderText(tr("Please enter the model ID"));
    m_idEdit->lineEdit()->setMaxLength(128);
    connect(m_idEdit, &DLineEdit::textChanged, this, &CustomModelGroup::modelChanged);

    m_testButton = new DCommandLinkButton(tr("Test"), this);
    DFontSizeManager::instance()->bind(m_testButton, DFontSizeManager::T8, QFont::Normal);
    connect(m_testButton, &DIconButton::clicked, this, &CustomModelGroup::requestCheck);

    m_deleteButton = new DIconButton(DStyle::SP_DeleteButton);
    m_deleteButton->setIconSize(QSize(17, 17));
    m_deleteButton->setFlat(true);
    m_deleteButton->hide();

    connect(m_deleteButton, &DIconButton::clicked, this, &CustomModelGroup::requestRemove);

    {
        auto idlay = new QHBoxLayout;
        idlay->setContentsMargins(0, 0, 0, 0);
        idlay->addWidget(m_idEdit);
        idlay->addWidget(m_testButton);
        idlay->addWidget(m_deleteButton);
        layout->addLayout(idlay);
    }

    m_nameEdit = new DLineEdit();
    m_nameEdit->setPlaceholderText(tr("Please enter the model name"));
    m_nameEdit->lineEdit()->setMaxLength(64);
    layout->addWidget(m_nameEdit);
}

void CustomModelGroup::setEditMode(bool edit)
{
    m_deleteButton->setVisible(edit);
    m_testButton->setVisible(!edit);
}

void CustomModelGroup::setReadOnly(bool b)
{
    m_nameEdit->setClearButtonEnabled(!b);
    m_nameEdit->lineEdit()->setReadOnly(b);

    m_idEdit->setClearButtonEnabled(!b);
    m_idEdit->lineEdit()->setReadOnly(b);
}

void CustomModelGroup::setEnableTest(bool enable)
{
    m_testButton->setEnabled(enable);
}

BuiltinModelItem::BuiltinModelItem(QWidget *parent) : DWidget(parent)
{
    setFixedHeight(36);

    auto lay = new QHBoxLayout;
    lay->setContentsMargins(10, 0, 10, 0);
    lay->setSpacing(10);
    m_checkBox = new DCheckBox(this);
    lay->addWidget(m_checkBox);
    lay->addStretch();

    m_testButton = new DCommandLinkButton(tr("Test"), this);
    DFontSizeManager::instance()->bind(m_testButton, DFontSizeManager::T8, QFont::Normal);
    m_testButton->hide();
    lay->addWidget(m_testButton);
    setLayout(lay);

    connect(m_testButton, &DIconButton::clicked, this, &BuiltinModelItem::requestCheck);
}

void BuiltinModelItem::setEnableCheck(bool enable)
{
    m_testButton->setEnabled(enable);
}

void BuiltinModelItem::setChecked(bool b)
{
    m_checkBox->setChecked(b);
}

bool BuiltinModelItem::isChecked() const
{
    return m_checkBox->isChecked();
}

#ifdef COMPILE_ON_QT6
void BuiltinModelItem::enterEvent(QEnterEvent *event)
#else
void BuiltinModelItem::enterEvent(QEvent *event)
#endif
{
    if (!m_hideTest)
        m_testButton->show();

    DWidget::enterEvent(event);
}

void BuiltinModelItem::leaveEvent(QEvent *event)
{
    m_testButton->hide();
    DWidget::leaveEvent(event);
}

void BuiltinModelItem::paintEvent(QPaintEvent *event)
{
    event->accept();

    DStylePainter painter(this);
    DStyleOptionBackgroundGroup option;
    option.init(this);
    //DStyleOptionBackgroundGroup::init 用的是this在parent中的坐标，这里需要改为0，0
    option.rect = QRect(QPoint(0, 0), option.rect.size());
    option.directions = Qt::Horizontal;
    option.position = DStyleOptionBackgroundGroup::ItemBackgroundPosition::OnlyOne;
    option.dpalette.setBrush(DPalette::ItemBackground, option.dpalette.brush(DPalette::ObviousBackground));
    painter.drawPrimitive(DStyle::PE_ItemBackground, option);
}
