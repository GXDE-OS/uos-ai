#include "localmodellistwidget.h"
#include "themedlable.h"
#include "app/serverwrapper.h"
#include "audiocontroler.h"
#include "localmodelitem.h"
#include "localmodelserver.h"
#include "yourongmodelitem.h"
#include "modelscopeitem.h"
#include "deepseekmodelitem.h"
#include "externalllm/modelhubwrapper.h"

#include <DLabel>
#include <DFontSizeManager>
#include <DBackgroundGroup>
#include <DGuiApplicationHelper>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLoggingCategory>

using namespace uos_ai;
DWIDGET_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

LocalModelListWidget::LocalModelListWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    onThemeTypeChanged();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &LocalModelListWidget::onThemeTypeChanged);
}

void LocalModelListWidget::initUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_pWidgetLabel = new ThemedLable(tr("Local model"));
    m_pWidgetLabel->setPaletteColor(QPalette::WindowText, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_pWidgetLabel, DFontSizeManager::T6, QFont::Medium);
    layout->addWidget(m_pWidgetLabel, 0, Qt::AlignLeft);

#ifdef ENABLE_LOCAL_MODEL
    layout->addWidget(embeddingPluginsWidget());
#ifdef ENABLE_ULLM
    layout->addWidget(yourong1_5BLLMWidget());
    layout->addWidget(yourong7BLLMWidget());
#endif
    layout->addWidget(deepseek_1_5BLLMWidget());
#endif

    layout->setSpacing(10);
    layout->addStretch();
}

void LocalModelListWidget::onThemeTypeChanged()
{
    qCDebug(logAIGUI) << "Theme type changed for LocalModelListWidget.";
    DPalette pl;

    if (m_pYouRong1_5B) {
        pl = m_pYouRong1_5B->palette();
        pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        m_pYouRong1_5B->setPalette(pl);
    }

    if (m_pYouRong7B) {
        pl = m_pYouRong7B->palette();
        pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        m_pYouRong7B->setPalette(pl);
    }

    if (m_pDeepseek1_5B) {
        pl = m_pDeepseek1_5B->palette();
        pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        m_pDeepseek1_5B->setPalette(pl);
    }

    if (m_pEmbeddingPluginsWidget) {
        pl = m_pEmbeddingPluginsWidget->palette();
        pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        m_pEmbeddingPluginsWidget->setPalette(pl);
    }
}

void LocalModelListWidget::onSetRedPointVisible(bool isShow)
{
    qCDebug(logAIGUI) << "Set red point visible. isShow:" << isShow;
    m_isShowRedPoint |= isShow;
    emit sigRedPointVisible(m_isShowRedPoint);
}

DBackgroundGroup *LocalModelListWidget::yourong1_5BLLMWidget()
{
    qCDebug(logAIGUI) << "Creating yourong1_5BLLMWidget.";
    YouRongModelItem *item = new YouRongModelItem(true, this);
    item->setText(tr("ULLM-1.5B"), tr("Once installed, you do not need an internet connection to use UOS AI."), 0.94);
    item->setMinimumHeight(60);
    item->setMaximumHeight(75);
    connect(item, &ModelScopeItem::sigRedPointVisible, this, &LocalModelListWidget::onSetRedPointVisible);

    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(item);

    m_pYouRong1_5B = new DBackgroundGroup(bgLayout, this);
    m_pYouRong1_5B->setContentsMargins(0, 0, 0, 0);

    return m_pYouRong1_5B;
}

DBackgroundGroup *LocalModelListWidget::yourong7BLLMWidget()
{
    qCDebug(logAIGUI) << "Creating yourong7BLLMWidget.";
    YouRongModelItem *item = new YouRongModelItem(false, this);
    item->setText(tr("ULLM-7B"), tr("Once installed, you do not need an internet connection to use UOS AI."), 4.4);
    item->setMinimumHeight(60);
    item->setMaximumHeight(75);
    connect(item, &ModelScopeItem::sigRedPointVisible, this, &LocalModelListWidget::onSetRedPointVisible);

    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(item);

    m_pYouRong7B = new DBackgroundGroup(bgLayout, this);
    m_pYouRong7B->setContentsMargins(0, 0, 0, 0);

    return m_pYouRong7B;
}

DBackgroundGroup *LocalModelListWidget::deepseek_1_5BLLMWidget()
{
    qCDebug(logAIGUI) << "Creating deepseek_1_5BLLMWidget.";
    DeepSeekModelItem *item = new DeepSeekModelItem(this);
    item->setText(tr("DeepSeek-R1-1.5B"), tr("Download from the open-source community at your own risk. Use it in UOS AI Assistant after installation."), 1.2);
    item->setMinimumHeight(60);
    item->setMaximumHeight(75);
    connect(item, &ModelScopeItem::sigRedPointVisible, this, &LocalModelListWidget::onSetRedPointVisible);

    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(item);

    m_pDeepseek1_5B = new DBackgroundGroup(bgLayout, this);
    m_pDeepseek1_5B->setContentsMargins(0, 0, 0, 0);

    return m_pDeepseek1_5B;
}

DBackgroundGroup *LocalModelListWidget::embeddingPluginsWidget()
{
    qCDebug(logAIGUI) << "Creating embeddingPluginsWidget.";
    LocalModelItem *item = new LocalModelItem(this);
    item->setText(tr("Embedding Plugins"), tr("After the model is installed, the System Assistant and the Personal Knowledge Assistantt can be run."));
    item->setAppName(PLUGINSNAME);
    item->setMinimumHeight(60);
    item->setMaximumHeight(75);
    connect(item, &LocalModelItem::sigRedPointVisible, this, &LocalModelListWidget::onSetRedPointVisible);

    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(item);

    m_pEmbeddingPluginsWidget = new DBackgroundGroup(bgLayout, this);
    m_pEmbeddingPluginsWidget->setContentsMargins(0, 0, 0, 0);

    return m_pEmbeddingPluginsWidget;
}

void LocalModelListWidget::updateLocalModelList()
{
    qCDebug(logAIGUI) << "Updating local model list.";

    bool isVisible = false;
    if (m_pYouRong1_5B) {
        auto yourong1_5BWidget = m_pYouRong1_5B->findChild<YouRongModelItem *>();
        if (yourong1_5BWidget) {
            yourong1_5BWidget->checkInstallStatus();
            isVisible = true;
        }
    }

    if (m_pYouRong7B) {
        auto yourong7BWidget = m_pYouRong7B->findChild<YouRongModelItem *>();
        if (yourong7BWidget) {
            yourong7BWidget->checkInstallStatus();
            isVisible = true;
        }
    }

    if (m_pDeepseek1_5B) {
        auto deepseekWidget = m_pDeepseek1_5B->findChild<ModelScopeItem *>();
        if (deepseekWidget) {
            deepseekWidget->checkInstallStatus();
            isVisible = true;
        }
    }

    if (m_pEmbeddingPluginsWidget) {
        auto embeddingPluginsWidget = m_pEmbeddingPluginsWidget->findChild<LocalModelItem *>();
        if (embeddingPluginsWidget) {
            embeddingPluginsWidget->stopTimer();
            embeddingPluginsWidget->checkInstallStatus();
            embeddingPluginsWidget->checkUpdateStatus();
            isVisible = true;
        }
    }

    this->setVisible(isVisible);

    this->setProperty("title", isVisible ? m_pWidgetLabel->text() : "");
}

void LocalModelListWidget::clearRedPoint()
{
    qCDebug(logAIGUI) << "Clearing red point.";
    if (m_pYouRong1_5B) {
        auto yourong1_5BWidget = m_pYouRong1_5B->findChild<YouRongModelItem *>();
        if (yourong1_5BWidget) {
            yourong1_5BWidget->saveNewHashFile();
        }
    }

    if (m_pYouRong7B) {
        auto yourong7BWidget = m_pYouRong7B->findChild<YouRongModelItem *>();
        if (yourong7BWidget) {
            yourong7BWidget->saveNewHashFile();
        }
    }

    if (m_pDeepseek1_5B) {
        auto deepseekWidget = m_pDeepseek1_5B->findChild<ModelScopeItem *>();
        if (deepseekWidget) {
            deepseekWidget->saveNewHashFile();
        }
    }

    if (m_pEmbeddingPluginsWidget) {
        auto embeddingPluginsWidget = m_pEmbeddingPluginsWidget->findChild<LocalModelItem *>();
        if (embeddingPluginsWidget) {
            embeddingPluginsWidget->saveUpdateVersion();
        }
    }

    m_isShowRedPoint = false;
    emit sigRedPointVisible(m_isShowRedPoint);
}

void LocalModelListWidget::stopDownload()
{
    qCDebug(logAIGUI) << "Stopping download for all models.";
    if (m_pYouRong1_5B) {
        auto yourong1_5BWidget = m_pYouRong1_5B->findChild<YouRongModelItem *>();
        if (yourong1_5BWidget)
            yourong1_5BWidget->stopDownload();
    }

    if (m_pYouRong7B) {
        auto yourong7BWidget = m_pYouRong7B->findChild<YouRongModelItem *>();
        if (yourong7BWidget)
            yourong7BWidget->stopDownload();
    }

    if (m_pDeepseek1_5B) {
        auto deepseekWidget = m_pDeepseek1_5B->findChild<ModelScopeItem *>();
        if (deepseekWidget) {
            deepseekWidget->stopDownload();
        }
    }
}

QString LocalModelListWidget::getTitleName()
{
    return m_pWidgetLabel->text();
}
