#include "privatemodellistwidget.h"
#include "modellistitem.h"
#include "addmodeldialog.h"
#include "themedlable.h"
#include "uosfreeaccounts.h"
#include "utils/util.h"

#include <DLabel>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLoggingCategory>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

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
    connect(m_pAddButton, &DCommandLinkButton::clicked, this, &PrivateModelListWidget::signalAddModel);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &PrivateModelListWidget::onThemeTypeChanged);
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

    auto items = m_pHasModelWidget->findChildren<ModelListItem *>();
    for (auto item : items) {
        item->setEditMode(editMode);
    }

    editMode ? m_pEditButton->setText(tr("Done")) : m_pEditButton->setText(tr("Delete"));
    m_pAddButton->setEnabled(!editMode);
}

DWidget *PrivateModelListWidget::noModelWidget()
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

DWidget *PrivateModelListWidget::hasModelWidget()
{
    DWidget *widget = new DWidget(this);
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_pHasModelWidget = new DBackgroundGroup(layout, this);
    m_pHasModelWidget->setContentsMargins(0, 0, 0, 0);
    m_pHasModelWidget->setItemSpacing(1);
    return m_pHasModelWidget;
}

void PrivateModelListWidget::setModelList(const QList<LLMServerProxy> &llmList)
{
    // 先将布局中的控件清空
    QList<QWidget *> childWidgets = m_pHasModelWidget->findChildren<QWidget *>();
    for (QWidget *childWidget : childWidgets) {
        childWidget->deleteLater();
    }

    for (auto llm : llmList) {
        if (llm.model == LLMChatModel::PRIVATE_MODEL)
            onAppendModel(llm);
    }

    adjustWidgetSize();
}

void PrivateModelListWidget::onAppendModel(const LLMServerProxy &llmServerProxy)
{
    auto item = m_pHasModelWidget->findChild<ModelListItem *>("ModelListItem_" + llmServerProxy.id);
    if (item) return;

    ModelListItem *newItem = new ModelListItem(llmServerProxy, this);
    connect(newItem, &ModelListItem::signalDeleteItem, this, &PrivateModelListWidget::removeModel, Qt::QueuedConnection);
    m_pHasModelWidget->layout()->addWidget(newItem);

    adjustWidgetSize();
    m_pEditButton->setText(tr("Delete"));
}

void PrivateModelListWidget::removeModel(const LLMServerProxy &llmServerProxy)
{
    qCInfo(logAIGUI) << "Remove model, id:" << llmServerProxy.id;
    auto item = m_pHasModelWidget->findChild<ModelListItem *>("ModelListItem_" + llmServerProxy.id);
    if (!item) return;

    m_pHasModelWidget->layout()->removeWidget(item);
    item->setParent(nullptr);
    delete item;

    adjustWidgetSize();
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


