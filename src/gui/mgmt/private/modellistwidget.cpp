#include "modellistwidget.h"
#include "modellistitem.h"
#include "addmodeldialog.h"
#include "themedlable.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <DLabel>

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
    layout->setMargin(0);
    layout->setSpacing(10);

    m_pEditButton = new DCommandLinkButton(tr("Delete"), this);
    DFontSizeManager::instance()->bind(m_pEditButton, DFontSizeManager::T8, QFont::Normal);
    m_pEditButton->setProperty("editMode", false);
    m_pEditButton->hide();
    m_pAddButton = new DCommandLinkButton(tr("Add"), this);
    DFontSizeManager::instance()->bind(m_pAddButton, DFontSizeManager::T8, QFont::Normal);

    QHBoxLayout *modelLayout = new QHBoxLayout;
    modelLayout->setContentsMargins(0, 0, 0, 0);
    modelLayout->setMargin(0);
    modelLayout->setSpacing(0);

    ThemedLable *label = new ThemedLable(tr("Online model"));
    label->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T5, QFont::Medium);
    modelLayout->addWidget(label);

    modelLayout->addStretch();
    modelLayout->addWidget(m_pEditButton);
    modelLayout->addWidget(m_pAddButton);

    layout->addLayout(modelLayout);
    layout->addWidget(noModelWidget());
    layout->addWidget(hasModelWidget());
}

void ModelListWidget::initConnect()
{
    connect(m_pEditButton, &DCommandLinkButton::clicked, this, &ModelListWidget::onEditButtonClicked);
    connect(m_pAddButton, &DCommandLinkButton::clicked, this, &ModelListWidget::signalAddModel);
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
}

void ModelListWidget::onEditButtonClicked()
{
    bool editMode = m_pEditButton->property("editMode").toBool();
    m_pEditButton->setProperty("editMode", !editMode);
    editMode = m_pEditButton->property("editMode").toBool();

    auto items = m_pHasModelWidget->findChildren<ModelListItem *>();
    for (auto item : items) {
        item->setEditMode(editMode);
    }

    editMode ? m_pEditButton->setText(tr("Done")) : m_pEditButton->setText(tr("Delete"));
    m_pAddButton->setEnabled(!editMode);
}

DWidget *ModelListWidget::noModelWidget()
{
    DWidget *widget = new DWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(10, 0, 10, 0);

    layout->addWidget(new DLabel(tr("None"), widget));
    layout->addStretch();

    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(widget);

    m_pNoModelWidget = new DBackgroundGroup(bgLayout, this);
    m_pNoModelWidget->setContentsMargins(0, 0, 0, 0);
    m_pNoModelWidget->setFixedHeight(36);

    return m_pNoModelWidget;
}

DWidget *ModelListWidget::hasModelWidget()
{
    DWidget *widget = new DWidget(this);
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setMargin(0);

    m_pHasModelWidget = new DBackgroundGroup(layout, this);
    m_pHasModelWidget->setContentsMargins(0, 0, 0, 0);
    m_pHasModelWidget->setItemSpacing(1);
    return m_pHasModelWidget;
}

void ModelListWidget::setModelList(const QList<LLMServerProxy> &llmList)
{
    // 先将布局中的控件清空
    QList<QWidget *> childWidgets = m_pHasModelWidget->findChildren<QWidget *>();
    for (QWidget *childWidget : childWidgets) {
        childWidget->deleteLater();
    }

    for (auto llm : llmList) {
        if (llm.model == LLMChatModel::LOCAL_TEXT2IMAGE) continue;
        onAppendModel(llm);
    }

    adjustWidgetSize();
}

void ModelListWidget::onAppendModel(const LLMServerProxy &llmServerProxy)
{
    auto item = m_pHasModelWidget->findChild<ModelListItem *>("ModelListItem_" + llmServerProxy.id);
    if (item) return;

    ModelListItem *newItem = new ModelListItem(llmServerProxy, this);
    connect(newItem, &ModelListItem::signalDeleteItem, this, &ModelListWidget::removeModel, Qt::QueuedConnection);
    m_pHasModelWidget->layout()->addWidget(newItem);

    adjustWidgetSize();
    m_pEditButton->setText(tr("Delete"));
}

void ModelListWidget::removeModel(const LLMServerProxy &llmServerProxy)
{
    auto item = m_pHasModelWidget->findChild<ModelListItem *>("ModelListItem_" + llmServerProxy.id);
    if (!item) return;

    m_pHasModelWidget->layout()->removeWidget(item);
    item->setParent(nullptr);
    delete item;

    adjustWidgetSize();
}

void ModelListWidget::adjustWidgetSize()
{
    if (m_pHasModelWidget->layout()->count() <= 0) {
        m_pNoModelWidget->show();
        m_pHasModelWidget->hide();
        m_pEditButton->hide();
        m_pEditButton->setProperty("editMode", false);
        m_pAddButton->setEnabled(true);
    } else {
        m_pNoModelWidget->hide();
        m_pHasModelWidget->show();
        m_pEditButton->show();
    }
    adjustSize();
}

void ModelListWidget::resetEditButton()
{
    if (m_pEditButton->property("editMode").toBool()) {
        onEditButtonClicked();
    }
}
