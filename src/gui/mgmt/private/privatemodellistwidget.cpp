#include "privatemodellistwidget.h"
#include "providerlistitem.h"
#include "modelsubitem.h"
#include "modifyproviderdialog.h"
#include "themedlable.h"
#include "utils/util.h"
#include "appdatabase.h"
#include "global_key_define.h"

#include <DLabel>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;
DWIDGET_USE_NAMESPACE

PrivateModelListWidget::PrivateModelListWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    initConnect();
    onThemeTypeChanged();
}

void PrivateModelListWidget::initUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_pEditButton = new DCommandLinkButton(tr("Delete"), this);
    DFontSizeManager::instance()->bind(m_pEditButton, DFontSizeManager::T8, QFont::Normal);
    m_pEditButton->setProperty("editMode", false);
    m_pEditButton->hide();
    m_pAddButton = new DCommandLinkButton(tr("Add"), this);
    DFontSizeManager::instance()->bind(m_pAddButton, DFontSizeManager::T8, QFont::Normal);

    QHBoxLayout *modelLayout = new QHBoxLayout;
    modelLayout->setContentsMargins(0, 0, 0, 0);
    modelLayout->setSpacing(0);

    m_pWidgetLabel = new ThemedLable(tr("Private deployment model"));
    m_pWidgetLabel->setPaletteColor(QPalette::WindowText, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_pWidgetLabel, DFontSizeManager::T6, QFont::Medium);
    modelLayout->addWidget(m_pWidgetLabel);

    modelLayout->addStretch();
    modelLayout->addWidget(m_pEditButton);
    modelLayout->addWidget(m_pAddButton);

    layout->addLayout(modelLayout);
    layout->addWidget(noModelWidget());
    layout->addWidget(hasModelWidget());
}

void PrivateModelListWidget::initConnect()
{
    connect(m_pEditButton, &DCommandLinkButton::clicked, this, &PrivateModelListWidget::onEditButtonClicked);
    connect(m_pAddButton, &DCommandLinkButton::clicked, this, &PrivateModelListWidget::onAddModel);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &PrivateModelListWidget::onThemeTypeChanged);
}

void PrivateModelListWidget::onEditItemClicked(const QString &id)
{
    QMap<QString, ProviderAccount> providers = AppDatabase::instance()->queryAllProviders();
    if (!providers.contains(id)) {
        qCWarning(logAIGUI) << "Provider not found. id:" << id;
        return;
    }

    QList<ModelAccountPtr> models = AppDatabase::instance()->queryAllModels();
    QList<ModelAccountPtr> providerModels;
    for (const auto &model : models) {
        if (model->account.id == id) {
            providerModels.append(model);
        }
    }

    ModifyProviderDialog dialog(providers[id], providerModels, true, this);
    connect(&dialog, &ModifyProviderDialog::dataChanged, this, &PrivateModelListWidget::refresh, Qt::QueuedConnection);
    dialog.exec();
}

void PrivateModelListWidget::onAddModel()
{
    ModifyProviderDialog dialog(true, this);
    connect(&dialog, &ModifyProviderDialog::dataChanged, this, &PrivateModelListWidget::refresh, Qt::QueuedConnection);
    dialog.exec();
}

void PrivateModelListWidget::onThemeTypeChanged()
{
    DPalette pl = m_pHasModelWidget->palette();
    pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pHasModelWidget->setPalette(pl);

    DPalette pl1 = m_pNoModelWidget->palette();
    pl1.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pNoModelWidget->setPalette(pl1);
}

void PrivateModelListWidget::onEditButtonClicked()
{
    qCInfo(logAIGUI) << "Edit button clicked. Current editMode:" << m_pEditButton->property("editMode").toBool();
    bool editMode = m_pEditButton->property("editMode").toBool();
    m_pEditButton->setProperty("editMode", !editMode);
    editMode = m_pEditButton->property("editMode").toBool();

    auto items = m_pHasModelWidget->findChildren<ProviderListItem *>();
    for (auto item : items) {
        item->setEditMode(editMode);
    }

    editMode ? m_pEditButton->setText(tr("Done")) : m_pEditButton->setText(tr("Delete"));
    m_pAddButton->setEnabled(!editMode);
}

DWidget *PrivateModelListWidget::noModelWidget()
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

DWidget *PrivateModelListWidget::hasModelWidget()
{
    if (!m_pHasModelWidget) {
        m_pHasModelWidget = new DWidget(this);
        auto layout = new QVBoxLayout(m_pHasModelWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(10);
    }

    return m_pHasModelWidget;
}

void PrivateModelListWidget::refresh()
{
    QMap<QString, ProviderAccount> providers = AppDatabase::instance()->queryAllProviders();
    QList<ModelAccountPtr> models = AppDatabase::instance()->queryAllModels();

    QList<QWidget *> childWidgets = m_pHasModelWidget->findChildren<QWidget *>();
    for (QWidget *childWidget : childWidgets) {
        childWidget->deleteLater();
    }

    QMap<QString, QList<ModelAccountPtr>> providerModels;
    for (const auto &model : models) {
        providerModels[model->account.id].append(model);
    }

    QMap<QString, ProviderAccount> privateProviders;
    for (const auto &provider : providers) {
        if (provider.provider.compare(STR_KEY_PRIVATE_NET, Qt::CaseInsensitive) != 0) {
            continue;
        }

        auto providerItem = new ProviderListItem(provider, this);
        connect(providerItem, &ProviderListItem::signalDeleteItem, this, &PrivateModelListWidget::removeProvider, Qt::QueuedConnection);
        connect(providerItem, &ProviderListItem::signalEditItem, this, &PrivateModelListWidget::onEditItemClicked, Qt::QueuedConnection);
        m_pHasModelWidget->layout()->addWidget(providerItem);

        if (providerModels.contains(provider.id)) {
            for (const auto &model : providerModels[provider.id]) {
                auto modelItem = new ModelSubItem(model, this);
                providerItem->addModelItem(modelItem);
            }
        }
    }

    adjustWidgetSize();
    qCInfo(logAIGUI) << "Private model list set. count:" << models.size();
}

void PrivateModelListWidget::removeProvider(const QString &id)
{
    auto item = m_pHasModelWidget->findChild<ProviderListItem *>("ProviderListItem_" + id);
    if (!item) return;

    m_pHasModelWidget->layout()->removeWidget(item);
    item->setParent(nullptr);
    delete item;

    adjustWidgetSize();
    qCInfo(logAIGUI) << "Provider removed. id:" << id;
}

void PrivateModelListWidget::adjustWidgetSize()
{
    if (m_pHasModelWidget->layout()->count() <= 0) {
        m_pNoModelWidget->show();
        m_pHasModelWidget->hide();
        m_pEditButton->hide();
        m_pEditButton->setProperty("editMode", false);
        m_pAddButton->setEnabled(true);
    }
    else {
        m_pNoModelWidget->hide();
        m_pHasModelWidget->show();
        m_pEditButton->show();
    }

    //adjustSize();//屏蔽防止重新布局条目过小的问题
}

void PrivateModelListWidget::resetEditButton()
{
    if (m_pEditButton->property("editMode").toBool()) {
        onEditButtonClicked();
    }
}

QString PrivateModelListWidget::getTitleName()
{
    return m_pWidgetLabel->text();
}


