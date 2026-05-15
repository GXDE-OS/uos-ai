#include "modellistwidget.h"
#include "providerlistitem.h"
#include "modelsubitem.h"
#include "themedlable.h"
#include "uosfreeaccounts.h"
#include "appdatabase.h"
#include "builtinprovider.h"
#include "global_key_define.h"
#include "modifyproviderdialog.h"
#include "model/modelvendor.h"

#include <DLabel>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

ModelListWidget::ModelListWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    initConnect();
    onThemeTypeChanged();
}

void ModelListWidget::initUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_pGetFreeAccountButton = new IconCommandLinkButton(tr("Get a free account"), IconPosition::Left, this);//Claim a free account
    m_pGetFreeAccountButton->setIcon(QIcon::fromTheme(":/assets/images/free_account.svg").pixmap(QSize(16, 16)));
    DFontSizeManager::instance()->bind(m_pGetFreeAccountButton, DFontSizeManager::T8, QFont::Normal);
    m_pGetFreeAccountButton->hide();

    m_pEditButton = new DCommandLinkButton(tr("Delete"), this);
    DFontSizeManager::instance()->bind(m_pEditButton, DFontSizeManager::T8, QFont::Normal);
    m_pEditButton->setProperty("editMode", false);
    m_pEditButton->hide();
    m_pAddButton = new DCommandLinkButton(tr("Add"), this);
    DFontSizeManager::instance()->bind(m_pAddButton, DFontSizeManager::T8, QFont::Normal);

    QHBoxLayout *modelLayout = new QHBoxLayout;
    modelLayout->setContentsMargins(0, 0, 0, 0);
    modelLayout->setSpacing(0);

    m_pWidgetLabel = new ThemedLable(tr("Online model"));
    m_pWidgetLabel->setPaletteColor(QPalette::WindowText, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_pWidgetLabel, DFontSizeManager::T6, QFont::Medium);
    modelLayout->addWidget(m_pWidgetLabel);

    modelLayout->addStretch();
    modelLayout->addWidget(m_pGetFreeAccountButton);
    modelLayout->addWidget(m_pEditButton);
    modelLayout->addWidget(m_pAddButton);

    layout->addLayout(modelLayout);
    layout->addWidget(noModelWidget());
    layout->addWidget(hasModelWidget());
}

void ModelListWidget::initConnect()
{
    connect(m_pGetFreeAccountButton, &IconCommandLinkButton::clicked, this, &ModelListWidget::onGetFreeAccount);
    connect(m_pEditButton, &DCommandLinkButton::clicked, this, &ModelListWidget::onEditButtonClicked);
    connect(m_pAddButton, &DCommandLinkButton::clicked, this, &ModelListWidget::onAddModel);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ModelListWidget::onThemeTypeChanged);
}

void ModelListWidget::onThemeTypeChanged()
{
    DPalette pl = m_pHasModelWidget->palette();
    pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pHasModelWidget->setPalette(pl);

    DPalette pl1 = m_pNoModelWidget->palette();
    pl1.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pNoModelWidget->setPalette(pl1);

    qCDebug(logAIGUI) << "Theme type changed for ModelListWidget.";
}

void ModelListWidget::onEditItemClicked(const QString &id)
{
    auto item = this->findChild<ProviderListItem *>("ProviderListItem_" + id);
    if (!item)
        return;

    ModifyProviderDialog dialog(item->getData(), item->models(), false, this);
    connect(&dialog, &ModifyProviderDialog::dataChanged, this, &ModelListWidget::refresh, Qt::QueuedConnection);

    dialog.exec();
}

void ModelListWidget::onAddModel()
{
    ModifyProviderDialog dialog(false, this);
    connect(&dialog, &ModifyProviderDialog::dataChanged, this, &ModelListWidget::refresh, Qt::QueuedConnection);
    dialog.exec();
}

void ModelListWidget::onEditButtonClicked()
{
    bool editMode = m_pEditButton->property("editMode").toBool();
    m_pEditButton->setProperty("editMode", !editMode);
    editMode = m_pEditButton->property("editMode").toBool();

    auto items = m_pHasModelWidget->findChildren<ProviderListItem *>();
    for (auto item : items) {
        item->setEditMode(editMode);
    }

    editMode ? m_pEditButton->setText(tr("Done")) : m_pEditButton->setText(tr("Delete"));
    m_pAddButton->setEnabled(!editMode);
    m_pGetFreeAccountButton->setEnabled(!editMode);
    qCInfo(logAIGUI) << "Edit button clicked. editMode:" << editMode;
}

DWidget *ModelListWidget::noModelWidget()
{
    if (!m_pNoModelWidget) {
        QHBoxLayout *bgLayout = new QHBoxLayout;
        bgLayout->setContentsMargins(10, 0, 10, 0);
        bgLayout->addWidget(new DLabel(tr("None")));

        m_pNoModelWidget = new DBackgroundGroup(bgLayout, this);
        m_pNoModelWidget->setContentsMargins(0, 0, 0, 0);
        m_pNoModelWidget->setFixedHeight(36);
    }

    return m_pNoModelWidget;
}

DWidget *ModelListWidget::hasModelWidget()
{
    if (!m_pHasModelWidget) {
        m_pHasModelWidget = new DWidget(this);
        auto layout = new QVBoxLayout(m_pHasModelWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(10);
    }

    return m_pHasModelWidget;
}

void ModelListWidget::refresh()
{
    QMap<QString, ProviderAccount> providers = AppDatabase::instance()->queryAllProviders();
    for (auto it = providers.begin(); it != providers.end();) {
        if (!BuiltinProvider::instance()->isProviderSupported(it->provider)) {
            qCWarning(logAIGUI) << "Provider not supported:" << it->id << it->name << it->provider;
            it = providers.erase(it);
            continue;
        }

        // 跳过私有模型
        if (it->provider.compare(STR_KEY_PRIVATE_NET, Qt::CaseInsensitive) == 0) {
            it = providers.erase(it);
            continue;
        }

        it++;
    }

    QList<ModelAccountPtr> models = AppDatabase::instance()->queryAllModels();

    QList<QWidget *> childWidgets = m_pHasModelWidget->findChildren<QWidget *>();
    for (QWidget *childWidget : childWidgets) {
        childWidget->deleteLater();
    }

    QMap<QString, QList<ModelAccountPtr>> providerModels;
    for (const auto &model : models) {
        providerModels[model->account.id].append(model);
    }

    for (const auto &provider : providers) {
        auto providerItem = new ProviderListItem(provider, this);
        connect(providerItem, &ProviderListItem::signalDeleteItem, this, &ModelListWidget::removeProvider, Qt::QueuedConnection);
        connect(providerItem, &ProviderListItem::signalEditItem, this, &ModelListWidget::onEditItemClicked, Qt::QueuedConnection);
        m_pHasModelWidget->layout()->addWidget(providerItem);

        if (providerModels.contains(provider.id)) {
            for (const auto &model : providerModels[provider.id]) {
                if (!ModelVendor::isValid(model)) {
                    qCWarning(logAIGUI) << "Invalid model, account" << model->id << "provider "<< model->account.id << model->account.provider
                                        << "model" << model->model.id << model->model.modelId;
                    continue;
                }
                auto modelItem = new ModelSubItem(model, this);
                providerItem->addModelItem(modelItem);
            }
        }
    }

    adjustWidgetSize();
    qCInfo(logAIGUI) << "Provider list set. count:" << providers.size();
}

void ModelListWidget::onGetFreeAccount()
{
    qCInfo(logAIGUI) << "Get free account button clicked.";
    emit signalGetFreeAccountClicked();
}

void ModelListWidget::removeProvider(const QString &id)
{
    auto item = m_pHasModelWidget->findChild<ProviderListItem *>("ProviderListItem_" + id);
    if (!item) return;

    m_pHasModelWidget->layout()->removeWidget(item);
    item->setParent(nullptr);
    delete item;

    adjustWidgetSize();
    qCInfo(logAIGUI) << "Provider removed. id:" << id;
}

void ModelListWidget::adjustWidgetSize()
{
    if (m_pHasModelWidget->layout()->count() <= 0) {
        m_pNoModelWidget->show();
        m_pHasModelWidget->hide();
        m_pEditButton->hide();
        m_pEditButton->setProperty("editMode", false);
        m_pEditButton->setText(tr("Delete"));
        m_pAddButton->setEnabled(true);
        m_pGetFreeAccountButton->setEnabled(true);
    } else {
        m_pNoModelWidget->hide();
        m_pHasModelWidget->show();
        m_pEditButton->show();
    }

    checkActivityExists();//判断免费活动是否超时
    //adjustSize();
}

void ModelListWidget::resetEditButton()
{
    if (m_pEditButton->property("editMode").toBool()) {
        onEditButtonClicked();
        qCDebug(logAIGUI) << "Reset edit button.";
    }
}

QString ModelListWidget::getTitleName()
{
    return m_pWidgetLabel->text();
}

void ModelListWidget::checkActivityExists()
{
    if (hasFreeAccount()) {
        m_pGetFreeAccountButton->hide();
        return;
    }

    if (m_watcher && m_watcher->isRunning())
        return;
    m_watcher.reset(new QFutureWatcher<QNetworkReply::NetworkError>);
#ifdef ENABLE_FREEACCOUNT
    {
#else
    if (UOSAI_NAMESPACE::Util::checkLanguage()) {
#endif
        QSharedPointer<UosFreeAccountActivity> tmpActivity(new UosFreeAccountActivity);
        QFuture<QNetworkReply::NetworkError> future = QtConcurrent::run([ = ] {
            return UosFreeAccounts::instance().freeAccountButtonDisplay("account", *tmpActivity.data());
        });
        m_watcher->setFuture(future);
        connect(m_watcher.data(), &QFutureWatcher<QNetworkReply::NetworkError>::finished, this, [ = ]() {
            m_hasActivity = *tmpActivity.data();
            if (hasFreeAccount()) {
                m_pGetFreeAccountButton->hide();
                return;
            }

            if (QNetworkReply::NoError == m_watcher.data()->future().result() && m_hasActivity.display) {
                m_pGetFreeAccountButton->show();
            } else
                m_pGetFreeAccountButton->hide();
        });
    }
}

void ModelListWidget::hiddenGetFreeAccountButton()
{
    m_pGetFreeAccountButton->hide();
}

bool ModelListWidget::hasFreeAccount() const
{
    if (m_pHasModelWidget->layout()->count() > 0) {
        auto items = m_pHasModelWidget->findChildren<ProviderListItem *>();
        for (auto item : items) {
            if (ModelVendor::isUosProvider(item->getData().provider)) {  // 免费账号
                return true;
            }
        }
    }
    return false;
}

