#include "localmodellistwidget.h"
#include "themedlable.h"
#include "localmodellistitem.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
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

#ifdef ENABLE_ASSISTANT
    layout->addWidget(embeddingPluginsWidget());
#ifndef COMPILE_ON_V20
    layout->addWidget(yourong1_5BLLMWidget());
    layout->addWidget(yourong7BLLMWidget());
#endif
    layout->addWidget(deepseek_1_5BLLMWidget());
#endif

    layout->addWidget(speechRecognitionWidget());
    layout->addWidget(textToImageWidget());
    layout->setSpacing(10);
    layout->addStretch();
}

void LocalModelListWidget::onThemeTypeChanged()
{
    qCDebug(logAIGUI) << "Theme type changed for LocalModelListWidget.";
    DPalette pl = m_pTextToImageWidget->palette();
    pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pTextToImageWidget->setPalette(pl);

    pl = m_pSpeechRecognitionWidget->palette();
    pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pSpeechRecognitionWidget->setPalette(pl);

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

    if (m_pLocalLlmWidget) {
        pl = m_pLocalLlmWidget->palette();
        pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        m_pLocalLlmWidget->setPalette(pl);
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

DBackgroundGroup *LocalModelListWidget::textToImageWidget()
{
    qCDebug(logAIGUI) << "Creating textToImageWidget.";
    LocalModelListItem *item = new LocalModelListItem(this);
    item->setText(tr("Text to image model"), tr("Use a local model to generate images. After turning it on, you can select the model in the model list in the chat interface."));
    item->setFixedHeight(78);
    item->setAppName("texttoimage");
    item->setSwitchChecked(DbWrapper::localDbWrapper().queryLlmByLlmid("TextToImage").isValid());
    connect(item, &LocalModelListItem::signalSwitchChanged, this, [this](bool b) {
        updateLocalModelSwitch(LocalModel::TextToImage, b);
    });
    connect(item, &LocalModelListItem::signalUninstall, [this]() {
        updateLocalModelList();
        updateLocalModelSwitch(LocalModel::TextToImage, false);
    });

    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(item);

    m_pTextToImageWidget = new DBackgroundGroup(bgLayout, this);
    m_pTextToImageWidget->setContentsMargins(0, 0, 0, 0);

    return m_pTextToImageWidget;
}

DBackgroundGroup *LocalModelListWidget::speechRecognitionWidget()
{
    qCDebug(logAIGUI) << "Creating speechRecognitionWidget.";
    LocalModelListItem *item = new LocalModelListItem(this);
    item->setText(tr("speech model"), tr("When turned on, the speech recognition and speech reading in the chat interface will use the local model without requiring an Internet connection."));
    item->setFixedHeight(78);
    item->setAppName("deepin-speech");
    item->setSwitchChecked(DbWrapper::localDbWrapper().getLocalSpeech());
    connect(item, &LocalModelListItem::signalSwitchChanged, this, [this](bool b) {
        updateLocalModelSwitch(LocalModel::SpeechRecognition, b);
        AudioControler::instance()->switchModel(b ? AudioControler::Local : AudioControler::NetWork);
    });
    connect(item, &LocalModelListItem::signalUninstall, [this]() {
        updateLocalModelList();
        updateLocalModelSwitch(LocalModel::SpeechRecognition, false);
        AudioControler::instance()->switchModel(AudioControler::NetWork);
    });

    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(item);

    m_pSpeechRecognitionWidget = new DBackgroundGroup(bgLayout, this);
    m_pSpeechRecognitionWidget->setContentsMargins(0, 0, 0, 0);

    return m_pSpeechRecognitionWidget;
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

DBackgroundGroup *LocalModelListWidget::localLLMWidget()
{
    qCDebug(logAIGUI) << "Creating localLLMWidget.";
    LocalModelItem *item = new LocalModelItem(this);
    item->setText(tr("UOS AI large model"), tr("Once installed, you do not need an internet connection to use UOS AI."));
    item->setAppName("uos-ai-llm");
    item->setMinimumHeight(60);
    item->setMaximumHeight(75);
    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(item);

    m_pLocalLlmWidget = new DBackgroundGroup(bgLayout, this);
    m_pLocalLlmWidget->setContentsMargins(0, 0, 0, 0);

    return m_pLocalLlmWidget;
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
    bool textToImageExists = checkModelExist(LocalModel::TextToImage);
    bool speechRecognitionExists = checkModelExist(LocalModel::SpeechRecognition);

    m_pTextToImageWidget->setVisible(textToImageExists);
    m_pSpeechRecognitionWidget->setVisible(speechRecognitionExists);

    auto yourong1_5BWidget = m_pYouRong1_5B->findChild<YouRongModelItem *>();
    if (yourong1_5BWidget) {
        yourong1_5BWidget->checkInstallStatus();
    }
    auto yourong7BWidget = m_pYouRong7B->findChild<YouRongModelItem *>();
    if (yourong7BWidget) {
        yourong7BWidget->checkInstallStatus();
    }

    auto deepseekWidget = m_pDeepseek1_5B->findChild<ModelScopeItem *>();
    if (deepseekWidget) {
        deepseekWidget->checkInstallStatus();
    }

    LocalModelItem *llmWidget = nullptr;
    if (m_pLocalLlmWidget) {
        llmWidget = m_pLocalLlmWidget->findChild<LocalModelItem *>();
        if (llmWidget) {
            llmWidget->stopTimer();
            llmWidget->checkInstallStatus();
            llmWidget->checkUpdateStatus();
        }
    }

    auto embeddingPluginsWidget = m_pEmbeddingPluginsWidget->findChild<LocalModelItem *>();
    if (embeddingPluginsWidget) {
        embeddingPluginsWidget->stopTimer();
        embeddingPluginsWidget->checkInstallStatus();
        embeddingPluginsWidget->checkUpdateStatus();
    }
    bool isVisible = textToImageExists || speechRecognitionExists || llmWidget || embeddingPluginsWidget || yourong1_5BWidget || yourong7BWidget;

    this->setVisible(isVisible);

    this->setProperty("title", isVisible ? m_pWidgetLabel->text() : "");
}

void LocalModelListWidget::clearRedPoint()
{
    qCDebug(logAIGUI) << "Clearing red point.";
    auto yourong1_5BWidget = m_pYouRong1_5B->findChild<YouRongModelItem *>();
    if (yourong1_5BWidget) {
        yourong1_5BWidget->saveNewHashFile();
    }
    auto yourong7BWidget = m_pYouRong7B->findChild<YouRongModelItem *>();
    if (yourong7BWidget) {
        yourong7BWidget->saveNewHashFile();
    }
    auto deepseekWidget = m_pDeepseek1_5B->findChild<ModelScopeItem *>();
    if (deepseekWidget) {
        deepseekWidget->saveNewHashFile();
    }

    auto embeddingPluginsWidget = m_pEmbeddingPluginsWidget->findChild<LocalModelItem *>();
    if (embeddingPluginsWidget) {
        embeddingPluginsWidget->saveUpdateVersion();
    }

    m_isShowRedPoint = false;
    emit sigRedPointVisible(m_isShowRedPoint);
}

void LocalModelListWidget::stopDownload()
{
    qCDebug(logAIGUI) << "Stopping download for all models.";
    auto yourong1_5BWidget = m_pYouRong1_5B->findChild<YouRongModelItem *>();
    if (yourong1_5BWidget)
        yourong1_5BWidget->stopDownload();

    auto yourong7BWidget = m_pYouRong7B->findChild<YouRongModelItem *>();
    if (yourong7BWidget)
        yourong7BWidget->stopDownload();

    auto deepseekWidget = m_pDeepseek1_5B->findChild<ModelScopeItem *>();
    if (deepseekWidget) {
        deepseekWidget->stopDownload();
    }
}

bool LocalModelListWidget::checkModelExist(const LocalModel model)
{
    qCDebug(logAIGUI) << "Checking if model exists. Model:" << model;
    if (model == LocalModel::TextToImage) {
        QFileInfo binInfo("/usr/bin/texttoimage");
        return binInfo.exists() && binInfo.isFile();
    } else if (model == LocalModel::SpeechRecognition) {
        QFileInfo binInfo("/usr/bin/speechrecognition");
        return binInfo.exists() && binInfo.isFile();
    }

    return false;
}

bool LocalModelListWidget::updateLocalModelSwitch(const LocalModel model, bool b)
{
    qCDebug(logAIGUI) << "Updating local model switch. Model:" << model << ", Switch:" << b;
    // 已卸载，但是界面没刷新
    if (b && !checkModelExist(model)) return false;

    if (LocalModel::TextToImage == model) {
        QString modelId = "TextToImage";
        if (b) {
            LLMServerProxy newAccount;
            newAccount.name = LLMServerProxy::llmName(LLMChatModel::LOCAL_TEXT2IMAGE);
            newAccount.id = modelId;
            newAccount.model = LLMChatModel::LOCAL_TEXT2IMAGE;
            newAccount.type = ModelType::LOCAL;
            AccountProxy proxy;
            proxy.apiKey = modelId;
            newAccount.account = proxy;
            if (!DbWrapper::localDbWrapper().appendLlm(newAccount)) return false;
            ServerWrapper::instance()->updateLLMAccount();
        } else {
            if (!DbWrapper::localDbWrapper().deleteLlm(modelId)) return false;
            ServerWrapper::instance()->updateLLMAccount();
        }
    } else if (LocalModel::SpeechRecognition == model) {
        return DbWrapper::localDbWrapper().updateLocalSpeech(b);
    }

    return true;
}

QString LocalModelListWidget::getTitleName()
{
    return m_pWidgetLabel->text();
}
