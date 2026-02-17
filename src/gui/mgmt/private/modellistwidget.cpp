#include "modellistwidget.h"
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
    m_pGetFreeAccountButton->setProperty("editMode", false);
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

    qCDebug(logAIGUI) << "Theme type changed for ModelListWidget.";
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
    m_pGetFreeAccountButton->setEnabled(!editMode);
    qCInfo(logAIGUI) << "Edit button clicked. editMode:" << editMode;
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
        if (llm.model == LLMChatModel::LOCAL_TEXT2IMAGE || llm.model == LLMChatModel::PRIVATE_MODEL) continue;
        onAppendModel(llm);
    }

    adjustWidgetSize();
    qCInfo(logAIGUI) << "Model list set. count:" << llmList.size();
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

void ModelListWidget::onGetFreeAccount()
{
    qCInfo(logAIGUI) << "Get free account button clicked.";
    emit signalGetFreeAccountClicked();
}

void ModelListWidget::removeModel(const LLMServerProxy &llmServerProxy)
{
    auto item = m_pHasModelWidget->findChild<ModelListItem *>("ModelListItem_" + llmServerProxy.id);
    if (!item) return;

    m_pHasModelWidget->layout()->removeWidget(item);
    item->setParent(nullptr);
    delete item;

    adjustWidgetSize();
    qCInfo(logAIGUI) << "Model removed. id:" << llmServerProxy.id;
}

void ModelListWidget::adjustWidgetSize()
{
    if (m_pHasModelWidget->layout()->count() <= 0) {
        m_pNoModelWidget->show();
        m_pHasModelWidget->hide();
        m_pEditButton->hide();
        m_pEditButton->setProperty("editMode", false);
        m_pAddButton->setEnabled(true);
        m_pGetFreeAccountButton->setEnabled(true);
    }
    else {
        m_pNoModelWidget->hide();
        m_pHasModelWidget->show();
        m_pEditButton->show();
    }

    checkActivityExists();//判断免费活动是否超时
    //adjustSize();//屏蔽防止重新布局条目过小的问题
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
    if (m_pHasModelWidget->layout()->count() > 0) {
        auto items = m_pHasModelWidget->findChildren<ModelListItem *>();
        for (auto item : items) {
            if (item->getData().type == FREE_NORMAL) {  // 普通免费账号
                m_pGetFreeAccountButton->hide();
                return;
            }
        }
    }

    if (m_watcher && m_watcher->isRunning())
            return;
    m_watcher.reset(new QFutureWatcher<QNetworkReply::NetworkError>);
#ifdef ENABLE_FREEACCOUNT
    {
#else
    if (UOSAI_NAMESPACE::Util::checkLanguage()) {
#endif
        QFuture<QNetworkReply::NetworkError> future = QtConcurrent::run([ = ] {
            return UosFreeAccounts::instance().freeAccountButtonDisplay("account", m_hasActivity);
        });
        m_watcher->setFuture(future);
        connect(m_watcher.data(), &QFutureWatcher<QNetworkReply::NetworkError>::finished, this, [ = ]() {
            if( m_pHasModelWidget->layout()->count() > 0) {
                auto items = m_pHasModelWidget->findChildren<ModelListItem *>();
                for (auto item : items) {
                    if (item->getData().type == FREE_NORMAL) {  // 普通免费账号
                        m_pGetFreeAccountButton->hide();
                        return;
                    }
                }
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

