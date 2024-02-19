#include "eaiproxy.h"
#include "eaiexecutor.h"
#include "eappaiprompt.h"
#include "eaicallbck.h"

#include <QScopedPointer>
#include <QDebug>

EAiProxy::EAiProxy(QObject *parent)
    : QObject{parent}
{
    connect(EAiExec(), &EAiExecutor::llmAccountLstChanged,
            this, &EAiProxy::llmAccountLstChanged);

    //Init audio singals
    connect(EAiExec(), &EAiExecutor::audioASRInComing,
            this, &EAiProxy::sigAudioASRStream);
    connect(EAiExec(), &EAiExecutor::audioASRError,
            this, &EAiProxy::sigAudioASRError);
    connect(EAiExec(), &EAiExecutor::playTTSError,
            this, &EAiProxy::sigPlayTTSError);
    connect(EAiExec(), &EAiExecutor::playTTSFinished,
            this, &EAiProxy::sigPlayTTSFinished);
    connect(EAiExec(), &EAiExecutor::audioOutputDeviceChanged,
            this, &EAiProxy::sigAudioOutputDevChanged);
    connect(EAiExec(), &EAiExecutor::audioInputDeviceChange,
            this, &EAiProxy::sigAudioInputDevChange);
    connect(EAiExec(), &EAiExecutor::audioSampleLevel,
            this, &EAiProxy::sigAudioSampleLevel);

    //Init TTP signals
    connect(EAiExec(), &EAiExecutor::chatConversationType,
            this, &EAiProxy::sigChatConversationType);

    connect(EAiExec(), &EAiExecutor::textToPictureFinish,
            this, &EAiProxy::sigText2PicFinish);
    //Try to stop all requst from this proxy when it's
    //going to be destructed
    connect(this, &EAiProxy::destroyed, this, [this]() {
        EAiExec()->clearAiRequest(this);
    });

    //Network signal
    connect(EAiExec(), &EAiExecutor::netStateChanged,
            this, &EAiProxy::sigNetStateChanged);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
    this, [this](DGuiApplicationHelper::ColorType themeType) {
        //Check if the theme's changed.
        if (m_themeType != themeType) {
            m_themeType = themeType;
            emit sigThemeChanged(m_themeType);
        }

        //Check if the active color is changed.
        QString activeColor = DGuiApplicationHelper::instance()->applicationPalette()
                              .color(DPalette::Normal, DPalette::Highlight)
                              .name(QColor::HexRgb);
        if (m_activeColor != activeColor) {
            m_activeColor = activeColor;
            emit sigActiveColorChanged(m_activeColor);
        }
    });

    m_themeType = DGuiApplicationHelper::instance()->themeType();

    m_activeColor = DGuiApplicationHelper::instance()->applicationPalette()
                    .color(DPalette::Normal, DPalette::Highlight)
                    .name(QColor::HexRgb);

    //Load toast message
    m_aiToastMessage.append("");
    m_aiToastMessage.append(QCoreApplication::translate("AiToastMessage", "Chat history cleared"));
    m_aiToastMessage.append(QCoreApplication::translate("AiToastMessage", "Copied successfully"));

    m_audioRecCounter.reset(new QTimer(this));
    connect(m_audioRecCounter.get(), &QTimer::timeout, this, [this]() {
        m_audioLenLimit--;

        if (m_audioLenLimit <= 10) {
            emit sigAudioCountDown(m_audioLenLimit);

            //Stop count at zero
            if (m_audioLenLimit == 0) {
                m_audioRecCounter->stop();
            }
        }
    });
}

QString EAiProxy::getNotifierName(AiAction act, int mode)
{
    Q_UNUSED(act);

    QString actionName("");

    actionName = (Mode::CacheMode == mode)
                 ? GET_NOTIFIER_NAME(AiReply)
                 : GET_SNOTIFIER_NAME(AiReply);

    return actionName;
}

QString EAiProxy::sendAiRequest(const QString &llmId, int llmType, int act, const QString &param, int mode)
{
    QString reqId("");

    if ((act > None && act < MaxAiAction)
            && (mode > Mode::None && mode < Mode::MaxMode)
            && (!param.isEmpty())
            /*&& (!llmId.isEmpty())*/ //Allow no account request
       ) {
        QScopedPointer<EAiPrompt> aiPrompt;
        QSharedPointer<EAiCallback> aiCallback;

        aiCallback.reset(new EAiCacheCallback(this));
        aiCallback->setDataMode(Mode(mode));

        //act param isn't used
        aiCallback->setNotifier(getNotifierName(None, mode));

        switch (act) {
        case Conversation:
            aiPrompt.reset(new EConversationPrompt(param));
            aiCallback->setOp(Conversation);
            break;
        default:
            qWarning() << " Don't implement operation:" << act;
            break;
        }

        //Set ai model type
        if (!aiPrompt.isNull()) {
            aiPrompt->setLLM(llmType);
        }
        //Send request to Executor queue.
        reqId = EAiExec()->sendAiRequst(llmId, aiPrompt->getAiPrompt(), aiCallback);
    } else {
        qWarning() << "Invalid parameter:"
                   << " llmId =" << llmId
                   << " act=" << act
                   << " param=" << param
                   << " mode=" << mode;
    }

    return reqId;
}

void EAiProxy::cancelAiRequest(const QString &id)
{
    EAiExec()->cancelAiRequst(id);
}

QString EAiProxy::currentLLMAccountId()
{
    return EAiExec()->currentLLMAccountId();
}

QString EAiProxy::queryLLMAccountList()
{
    return EAiExec()->queryLLMAccountList();
}

bool EAiProxy::setCurrentLLMAccountId(const QString &id)
{
    return EAiExec()->setCurrentLLMAccountId(id);
}

QString EAiProxy::getAiFAQ()
{
    return EAiExec()->getRandomAiFAQ();
}

void EAiProxy::launchLLMConfigWindow(bool showAddllmPage)
{
    return EAiExec()->launchLLMConfigWindow(showAddllmPage);
}

void EAiProxy::launchAboutWindow()
{
    return EAiExec()->launchAboutWindow();
}

void EAiProxy::showToast(int type)
{
    auto index = AiToast(type);

    if (index > 0 && type < m_aiToastMessage.size()) {
        EAiExec()->showToast(m_aiToastMessage[index]);
    }
}

void EAiProxy::closeChatWindow()
{
    EAiExec()->closeChatWindow();
}

bool EAiProxy::isAudioInputAvailable()
{
    return EAiExec()->isAudioInputAvailable();
}

bool EAiProxy::isAudioOutputAvailable()
{
    return EAiExec()->isAudioOutputAvailable();
}

bool EAiProxy::startRecorder()
{
    //Reset audio limit at start.
    m_audioLenLimit = AUDIO_LENGTH;

    //Stop time anyway.
    m_audioRecCounter->stop();
    m_audioRecCounter->start(1000);

    return EAiExec()->startRecorder();
}

bool EAiProxy::stopRecorder()
{
    //Stop countdown timer
    m_audioRecCounter->stop();

    return EAiExec()->stopRecorder();
}

bool EAiProxy::playTextAudio(const QString &id, const QString &text, bool isEnd)
{
    return EAiExec()->playTextAudio(id, text, isEnd);
}

bool EAiProxy::stopPlayTextAudio()
{
    return EAiExec()->stopPlayTextAudio();
}

void EAiProxy::playSystemSound(int effId)
{
    return EAiExec()->playSystemSound(effId);
}

void EAiProxy::logAiChatRecord(const QString &reqId,
                               const QString &question,
                               const QString &anwser,
                               bool isRetry,
                               int err,
                               const QString &errorInfo,
                               int actionType,
                               const QString &llmIcon,
                               const QString &llmName)
{
    return EAiExec()->logChatRecord(
               reqId, question, QStringList(anwser),
               isRetry, err, errorInfo,
               actionType,
               llmIcon, llmName);
}

void EAiProxy::logAiChatRecord(const QString &reqId,
                               const QString &question,
                               const QStringList &anwser,
                               bool isRetry,
                               int err,
                               const QString &errorInfo,
                               int actionType,
                               const QString &llmIcon,
                               const QString &llmName)
{
    return EAiExec()->logChatRecord(
               reqId, question, anwser,
               isRetry, err, errorInfo,
               actionType,
               llmIcon, llmName);
}

QString EAiProxy::getAiChatRecords(bool lastRec)
{
    return EAiExec()->getChatRecords(lastRec);
}

void EAiProxy::clearAiChatRecords()
{
    return EAiExec()->clearChatRecords();
}

bool EAiProxy::saveImageAs(const QString &filePath)
{
    return EAiExec()->saveImageAs(filePath);
}

bool EAiProxy::previewImage(const QString &filePath)
{
    return EAiExec()->previewImage(filePath);
}

void EAiProxy::copyImage2Clipboard(const QString &filePath)
{
    return EAiExec()->copyImg2Clipboard(filePath);
}

bool EAiProxy::isNetworkAvailable()
{
    return EAiExec()->isNetworkAvailable();
}

QJsonObject EAiProxy::loadTranslations()
{
    static QJsonObject translations;
    if (!translations.isEmpty())
        return translations;

    translations["Go to configuration"] = tr("Go to configuration");
    translations["No account"] = tr("No account");
    translations["Input question"] = tr("Input question");
    translations["The content generated by AI is for reference only, please pay attention to the accuracy of the information."] = tr("The content generated by AI is for reference only, please pay attention to the accuracy of the information.");
    translations["Welcome to UOS AI"] = tr("Welcome to UOS AI");
    translations["Here are some of the things UOS AI can help you do"] = tr("Here are some of the things UOS AI can help you do");
    translations["Stop"] = tr("Stop");
    translations["Regenerate"] = tr("Regenerate");
    translations["Clear conversation history"] = tr("Clear conversation history");
    translations["Please connect the microphone and try again"] = tr("Please connect the microphone and try again");
    translations["Chat history cleared"] = tr("Chat history cleared");
    translations["No account"] = tr("No account");
    translations["Click to start/stop recording"] = tr("Click to start/stop recording");
    translations["Listening"] = tr("Listening");
    translations["Sleeping"] = tr("Sleeping");
    translations["Microphone not detected"] = tr("Microphone not detected");
    translations["Connection failed, click to try again"] = tr("Connection failed, click to try again");
    translations["Click on the animation or Ctrl+Super+C to activate"] = tr("Click on the animation or Ctrl+Super+C to activate");
    translations["Voice input is temporarily unavailable, please check the network!"] = tr("Voice input is temporarily unavailable, please check the network!");
    translations["Connection failed, please check the network."] = tr("Connection failed, please check the network.");
    translations["Voice conversation"] = tr("Voice conversation");
    translations["Click the animation or press Enter to send"] = tr("Click the animation or press Enter to send");
    translations["Stop recording after %1 seconds"] = tr("Stop recording after %1 seconds");
    translations["Thinking"] = tr("Thinking");
    translations["Click animation to interrupt"] = tr("Click animation to interrupt");
    translations["Answering"] = tr("Answering");
    translations["Your free account quota has been exhausted, please configure your model account to continue using it."] = tr("Your free account quota has been exhausted, please configure your model account to continue using it.");
    translations["Your free account has expired, please configure your model account to continue using it."] = tr("Your free account has expired, please configure your model account to continue using it.");
    translations["UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first."] = tr("UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
    translations["Activate"] = tr("Activate");
    translations["Voice input"] = tr("Voice input");
    translations["Voice broadcast is temporarily unavailable, please check the network!"] = tr("Voice broadcast is temporarily unavailable, please check the network!");
    translations["Turn off voice conversation"] = tr("Turn off voice conversation");
    translations["The picture has been generated, please switch to the chat interface to view it."] = tr("The picture has been generated, please switch to the chat interface to view it.");
    translations["No account, please configure an account"] = tr("No account, please configure an account");
    translations["Answer each question up to 5 times"] = tr("Answer each question up to 5 times");
    translations["Copied successfully"] = tr("Copied successfully");
    translations["Sound output device not detected"] = tr("Sound output device not detected");
    translations["The sound output device is not detected, please check and try again!"] = tr("The sound output device is not detected, please check and try again!");
    translations["Settings"] = tr("Settings");
    translations["About"] = tr("About");

    return translations;
}
