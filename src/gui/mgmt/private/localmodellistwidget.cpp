#include "localmodellistwidget.h"
#include "themedlable.h"
#include "localmodellistitem.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "audiocontroler.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <DLabel>
#include <DFontSizeManager>
#include <DBackgroundGroup>
#include <DGuiApplicationHelper>

LocalModelListWidget::LocalModelListWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    onThemeTypeChanged();
}

void LocalModelListWidget::initUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setMargin(0);
    layout->setSpacing(0);

    ThemedLable *label = new ThemedLable(tr("Local model"));
    label->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T5, QFont::Medium);

    layout->addWidget(label, 0, Qt::AlignLeft);
    layout->addSpacing(20);
    layout->addWidget(textToImageWidget());
    layout->addSpacing(1);
    layout->addWidget(speechRecognitionWidget());
    layout->addStretch();
}

void LocalModelListWidget::onThemeTypeChanged()
{
    DPalette pl = m_pTextToImageWidget->palette();
    pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pTextToImageWidget->setPalette(pl);

    pl = m_pSpeechRecognitionWidget->palette();
    pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pSpeechRecognitionWidget->setPalette(pl);
}

DBackgroundGroup *LocalModelListWidget::textToImageWidget()
{
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

void LocalModelListWidget::updateLocalModelList()
{
    bool textToImageExists = checkModelExist(LocalModel::TextToImage);
    bool speechRecognitionExists = checkModelExist(LocalModel::SpeechRecognition);

    m_pTextToImageWidget->setVisible(textToImageExists);
    m_pSpeechRecognitionWidget->setVisible(speechRecognitionExists);
    this->setVisible(textToImageExists || speechRecognitionExists);
}

bool LocalModelListWidget::checkModelExist(const LocalModel model)
{
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
