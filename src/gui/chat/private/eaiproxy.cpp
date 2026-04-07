#include "eaiproxy.h"
#include "eaiexecutor.h"
#include "eappaiprompt.h"
#include "eaicallbck.h"
#include "epptwebview.h"
#include "eposterwebview.h"
#include "eparserdocument.h"
#include "localmodelserver.h"
#include "easyncworker.h"
#include "esystemcontext.h"
#include "systemeventwatcher.h"

#include <QScopedPointer>
#include <QApplication>
#include <QClipboard>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QLoggingCategory>
#include <QDBusReply>
#include <QFileInfo>
#include <report/screenshotclickedpoint.h>
#include <report/mcpchatpoint.h>
#include <report/eventlogutil.h>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)
Q_DECLARE_LOGGING_CATEGORY(logAIVUE)

using namespace uos_ai;

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
    connect(EAiExec(), &EAiExecutor::pptCreateSuccess,
            this, &EAiProxy::sigPPTCreateSuccess);
    connect(EAiExec(), &EAiExecutor::pptChangeSuccess,
            this, &EAiProxy::sigPPTChangeSuccess);
    connect(EAiExec(), &EAiExecutor::posterCreateSuccess,
            this, &EAiProxy::sigPosterCreateSuccess);
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
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));

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
                qCDebug(logAIGUI) << "Audio recording time limit reached";
            }
        }
    });

    connect(EAiExec(), &EAiExecutor::knowledgeBaseExistChanged, this, &EAiProxy::sigKnowledgeBaseStatusChanged);
    connect(EAiExec(), &EAiExecutor::knowledgeBaseFAQGenFinished, this, &EAiProxy::sigKnowledgeBaseFAQGenFinished);

    connect(EAiExec(), &EAiExecutor::localLLMStatusChanged, this, &EAiProxy::sigLocalLLMStatusChanged);
    connect(EAiExec(), &EAiExecutor::embeddingPluginsStatusChanged, this, &EAiProxy::sigEmbeddingPluginsStatusChanged);

    // 拖拽文档解析
    connect(EAiExec(), &EAiExecutor::docSummaryParsingStart, this, &EAiProxy::sigDocSummaryParsingStart);
    connect(EAiExec(), &EAiExecutor::docSummaryForOffice, this, &EAiProxy::sigDocSummaryForOffice);
    connect(EAiExec(), &EAiExecutor::docDragInViewParserResult, this, &EAiProxy::sigDocSummaryParserResult);
    // API: openFile() -> Result
    connect(EAiExec(), &EAiExecutor::openFileFromPathResult, this, &EAiProxy::sigOpenFileFromPathResult);

    connect(EAiExec(), &EAiExecutor::sigAppendWordWizardConv, this, &EAiProxy::sigAppendWordWizardConv);

    connect(EAiExec(), &EAiExecutor::previewReference, this, &EAiProxy::sigPreviewReference);

    connect(EAiExec(), &EAiExecutor::sigOverrideQues, this, &EAiProxy::sigOverrideQues);
    connect(EAiExec(), &EAiExecutor::sigAssistantListChanged, this, &EAiProxy::sigAssistantListChanged);

    connect(EAiExec(), &EAiExecutor::sigGetNewHistoryList, this, &EAiProxy::sigGetNewHistoryList);
    connect(EAiExec(), &EAiExecutor::sigHideHistoryList, this, &EAiProxy::sigHideHistoryList);

    connect(EAiExec(), &EAiExecutor::sigToShowPromptWindow, this, &EAiProxy::sigToShowPromptWindow);
    connect(EAiExec(), &EAiExecutor::sigToChangeFreeAccountGuide, this, &EAiProxy::sigToChangeFreeAccountGuide);

    connect(EAiExec(), &EAiExecutor::sigMainContentBackgroundColor, this, [this](QString color){
        m_backgroundColor = color;
        emit sigMainContentBackgroundColor(color);
    });

    connect(EAiExec(), &EAiExecutor::sigInputFocus, this, &EAiProxy::sigInputFocus);

    connect(EAiExec(), &EAiExecutor::sigStartScreenshot, this, &EAiProxy::startScreenshot);
    connect(EAiExec(), &EAiExecutor::sigShowTip, this, &EAiProxy::sigShowTip);

    connect(EAiExec(), &EAiExecutor::sigWordWizardAsk, this, &EAiProxy::sigWordWizardAsk);

    connect(EAiExec(), &EAiExecutor::sigClaimUsageResult, this, &EAiProxy::sigGetFreeCreditsResult);
    connect(EAiExec(), &EAiExecutor::sigClaimAgain, this, &EAiProxy::sigIsGotFreeCredits);

    connect(EAiExec(), &EAiExecutor::sigIconThemeChanged, this, &EAiProxy::sigIconThemeChanged);

    connect(EAiExec(), &EAiExecutor::sigDownloadFileFinished, this, &EAiProxy::sigDownloadFileFinished);
    connect(EAiExec(), &EAiExecutor::sigActiveChatFromDigitalImage, this, &EAiProxy::sigActiveChatFromDigitalImage);

    // Connect system event watcher to forward system events to web frontend
    connect(&SystemEventWatcher::instance(), &SystemEventWatcher::sigSystemEvent,
            this, &EAiProxy::sigSystemEvent);

    initAsyncWorkerConn();
}

void EAiProxy::onUpdateSystemFont(const QFont &font)
{
    QFontInfo fontInfo(font);
    setFontInfo(fontInfo.family(), fontInfo.pixelSize());
    emit sigFontChanged(fontInfo.family(), fontInfo.pixelSize());
}

bool EAiProxy::isKnowledgeBaseExist()
{
    return EAiExec()->isKnowledgeBaseExist();
}

bool EAiProxy::isEmbeddingPluginsExist()
{
    return EAiExec()->isEmbeddingPluginsExist();
}

void EAiProxy::configureKnowledgeBase()
{
    return EAiExec()->launchLLMConfigWindow(false);
}

void EAiProxy::installEmbeddingPlugins()
{
    return EAiExec()->openInstallWidget(PLUGINSNAME);
}

void EAiProxy::onDocSummarySelect()
{
    EAiExec()->documentSummarySelect();
}

void EAiProxy::onDocSummaryForOfficeSelect(int category)
{
    EAiExec()->documentSummaryForOfficeSelect(category);
}

QString EAiProxy::processClipboardData()
{
    return EAiExec()->processClipboardData();
}

void EAiProxy::onDocSummaryParsing(const QString &id, const QString &docPath)
{
    EAiExec()->documentSummaryParsing(id, docPath);
}

void EAiProxy::openFile(const QString &filePath)
{
    EAiExec()->openFile(filePath);
}

void EAiProxy::openUrl(const QString &url)
{
    EAiExec()->openUrl(url);
}

void EAiProxy::appendWordWizardConv(int type)
{
    EAiExec()->appendWordWizardConv(type);
}

void EAiProxy::appendWordWizardQuestion(int type)
{
    EAiExec()->appendWordWizardQuestion(type);
}

void EAiProxy::webViewLoadFinished()
{
    EAiExec()->webViewLoadFinished();
}

void EAiProxy::previewRefDoc(const QString &docPath, const QStringList &docContents)
{
    EAiExec()->previewRefDoc(docPath, docContents);
}

QString EAiProxy::getInstList()
{
    return EAiExec()->getInstList();
}

void EAiProxy::sendPPTOutline(const QString &content)
{
    EPPTWebView *pptweb = ESystemContext::createWebWindow<EPPTWebView>(content, "2", "0");
    connect(pptweb, &EPPTWebView::sigPPTCreated,
            EAiExec(), &EAiExecutor::onPPTCreated);
    connect(pptweb, &EPPTWebView::sigPPTChanged,
            EAiExec(), &EAiExecutor::onPPTChanged);
}

void EAiProxy::editPPT(const QString &content)
{
    EPPTWebView *pptweb = ESystemContext::createWebWindow<EPPTWebView>(content, "1", "0");
    connect(pptweb, &EPPTWebView::sigPPTCreated,
            EAiExec(), &EAiExecutor::onPPTCreated);
    connect(pptweb, &EPPTWebView::sigPPTChanged,
            EAiExec(), &EAiExecutor::onPPTChanged);
}

void EAiProxy::downloadPPT(const QString &id)
{
    EPPTWebView *pptweb = ESystemContext::createWebWindow<EPPTWebView>("", "1", id);
    connect(pptweb, &EPPTWebView::sigPPTCreated,
            EAiExec(), &EAiExecutor::onPPTCreated);
    connect(pptweb, &EPPTWebView::sigPPTChanged,
            EAiExec(), &EAiExecutor::onPPTChanged);
}

void EAiProxy::editPoster(const QString &content)
{
    QRegularExpression regex("ID：(\\d+)");
    QRegularExpressionMatch match = regex.match(content);

    if (match.hasMatch()) {
        // 提取ID
        QString id = match.captured(1);
        EPosterWebView *posterweb = ESystemContext::createWebWindow<EPosterWebView>(EPosterWebView::WebSitePage::ModifyParameters, id);
        connect(posterweb, &EPosterWebView::sigPosterCreated, EAiExec(), &EAiExecutor::onPosterCreated);
    } else {
        qCWarning(logAIGUI) << "not found id.";
    }
}

void EAiProxy::genePoster(const QString &content)
{
    QRegularExpression regex("ID：(\\d+)");
    QRegularExpressionMatch match = regex.match(content);

    if (match.hasMatch()) {
        // 提取ID
        QString id = match.captured(1);
        EPosterWebView *posterweb = ESystemContext::createWebWindow<EPosterWebView>(EPosterWebView::WebSitePage::GenerateWorks, id);
        connect(posterweb, &EPosterWebView::sigPosterCreated, EAiExec(), &EAiExecutor::onPosterCreated);
    } else {
        qCWarning(logAIGUI) << "not found id.";
    }
}

void EAiProxy::downloadPoster(const QString &id)
{
    EPosterWebView *posterweb = ESystemContext::createWebWindow<EPosterWebView>(EPosterWebView::WebSitePage::ModifyWork, id);
    connect(posterweb, &EPosterWebView::sigPosterCreated, EAiExec(), &EAiExecutor::onPosterCreated);
}

bool EAiProxy::isAgentSupported()
{
#ifdef ENABLE_AGENT_PLUGIN
    return true;
#else
    return false;
#endif
}

void EAiProxy::openAppstore(const QString &id)
{
    if (id == "agent") {
        auto con = QDBusConnection::sessionBus();
        QDBusMessage msg = QDBusMessage::createMethodCall("com.home.appstore.client", "/com/home/appstore/client",
                                       "com.home.appstore.client", "openBusinessUri");

        QVariantList args;
        args << QVariant::fromValue(QString("tab/topAiAgentApp"));
        msg.setArguments(args);

        con.asyncCall(msg);
    }
}

void EAiProxy::onAsyncWorkerFinished(int type)
{
    emit EAsync()->sigWorkerFinished(type);
}

void EAiProxy::rateAnwser(const int questionIdx, const int answerIdx, int rate, const QString &extJson)
{
    EAiExec()->rateAnwser(questionIdx, answerIdx, rate, extJson);
}

void EAiProxy::setTitleBarStatus(bool status)
{
    EAiExec()->setTitleBarStatus(status);
}

bool EAiProxy::showWarningDialog(const QString assistantId, const QString conversationId, const QString msg, bool isDelete, bool isLlmDelete ,bool isAllConvDelete)
{
    return EAiExec()->showWarningDialog(assistantId, conversationId, msg, isDelete, isLlmDelete, isAllConvDelete);
}

bool EAiProxy::showRemoveFileDialog(const QString &message)
{
    return EAiExec()->showRemoveFileDialog(message);
}

bool EAiProxy::showAllowUploadFilesAlert(int modelType, const QString &modelDisplayName, bool searchOnline)
{
    return EAiExec()->showAllowUploadFilesAlert(modelType, modelDisplayName, searchOnline);
}

void EAiProxy::updateUpdatePromptDB(bool isClicked)
{
    EAiExec()->updateUpdatePromptDB(isClicked);
}

void EAiProxy::updateUpdateFreeAccountGuideDB(bool isClicked)
{
    EAiExec()->updateUpdateFreeAccountGuideDB(isClicked);
}

void EAiProxy::writeVueLog(int level, const QString &msg)
{
    switch (level)
    {
    case 0:
        qCDebug(logAIVUE) << msg;
        break;
    case 1:
        qCInfo(logAIVUE) << msg;
        break;
    case 2:
        qCWarning(logAIVUE) << msg;
        break;
    case 3:
        qCCritical(logAIVUE) << msg;
        break;
    default:
        break;
    }
}

void EAiProxy::installUosAiAgent()
{
    return EAiExec()->openInstallWidget(UOSAIAGENTNAME);
}

bool EAiProxy::isEnableMcp()
{
#ifdef ENABLE_MCP
    return true;
#else
    return false;
#endif
}

bool EAiProxy::getThirdPartyMcpAgreement()
{
    return  EAiExec()->getThirdPartyMcpAgreement();
}

bool EAiProxy::isInstallUosAiAgent(const QString &agentName)
{
    return  EAiExec()->isInstallUosAiAgent(agentName);
}

void EAiProxy::setInputFileSize(int inputFileSize)
{
    return EAiExec()->setInputFileSize(inputFileSize);
}

void EAiProxy::startScreenshot()
{
    // tid:1001600012 event:screenshot_clicked
    ReportIns()->writeEvent(report::ScreenShotClickedPoint().assemblingData());
    switch(isEnableScreenshot()) {
    case -1:
    {
        return;
    }

    case 0:
    {
        break;
    }

    case 1:
    {
        return EAiExec()->showUpdateDialog(tr("Update the UOS Screen Recorder to version 6.6 or later and restart your computer to enable Screenshot Q&A."),"deepin-screen-recorder");
    }

    case 2:
    {
        return;
    }

    }

    return EAiExec()->startScreenshot();
}

int EAiProxy::isEnableScreenshot()
{
    // -1: 截图按钮不显示
    // 0: 截图功能可用
    // 1: 需要升级高版本截图
    // 2: 正在录屏
#ifdef COMPILE_ON_V25
    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.isConnected()) {
        qCWarning(logAIGUI) << "Can not connect sessionBus.";
        return 1;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.Screenshot",
                                                     "/com/deepin/Screenshot",
                                                     "org.freedesktop.DBus.Introspectable",
                                                     "Introspect");

    QDBusReply<QString> screenShotReply = connection.call(msg);
    if (screenShotReply.isValid() && screenShotReply.value().contains("CustomScreenshot")) {
        qCInfo(logAIGUI) << "The CustomScreenshot interface exists.";
    } else {
        qCWarning(logAIGUI) << "The CustomScreenshot interface is not found.";
        return 1;
    }

    QDBusInterface dbusInterface("com.deepin.ScreenRecorder",
                                 "/com/deepin/ScreenRecorder",
                                 "org.freedesktop.DBus.Properties",
                                 QDBusConnection::sessionBus());

    QDBusReply<QDBusVariant> screenRecorderReply = dbusInterface.call("Get", "com.deepin.ScreenRecorder", "IsRecording");

    if (screenRecorderReply.isValid()) {
        return screenRecorderReply.value().variant().toBool() ? 2 : 0;
    }
    return 0;
#endif
    return -1;
}

void EAiProxy::setConversationMode(int mode)
{
    EAiExec()->setConversationMode(mode);
}

bool EAiProxy::isFileExist(QString filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }

    QFileInfo fileInfo(filePath);
    return fileInfo.exists() && fileInfo.isFile();
}

bool EAiProxy::isEnableKnowledgebase()
{
#ifdef ENABLE_ASSISTANT
    return true;
#else
    return false;
#endif
}

void EAiProxy::showKnowledgeBaseErrorDialog(const int type)
{
    EAiExec()->showKnowledgeBaseErrorDialog(type);
}

void EAiProxy::editQuestionToFileSummary(const QStringList fileLists)
{
    EAiExec()->onDocSummaryDragInView(fileLists, QString());
}

bool EAiProxy::showLostFileWarningDlg(const QString lostFileList)
{
    return EAiExec()->showLostFileWarningDlg(lostFileList);
}

bool EAiProxy::showInstallUosAIAgentDlg()
{
    return EAiExec()->showInstallUosAIAgentDlg();
}

void EAiProxy::mcpDataUpload(const QString &mcpToUse)
{
    QString mcpName;
    int dotCount = mcpToUse.count('.');
    if (dotCount == 1) {
        // Split the string by dot and take the first part as mcpName
        mcpName = mcpToUse.split('.').first();
    } else {
        // If no dot or multiple dots exist, use the original string
        mcpName = mcpToUse;
    }
    // tid:1001600014 event:mcp_chat
    ReportIns()->writeEvent(report::MCPChatPoint(mcpName).assemblingData());
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

void EAiProxy::initAsyncWorkerConn()
{
    connect(EAsync(), &EAsyncWorker::sigAsyncWorker, this, &EAiProxy::sigAsyncWorker);
}

QString EAiProxy::sendAiRequest(const QString &llmId, int llmType, int act, const QString &param, int mode)
{
    QString reqId("");

    if ((act > None && act < MaxAiAction)
            && (mode > Mode::None && mode < Mode::MaxMode)
            && (!param.isEmpty())
            /*&& (!llmId.isEmpty())*/ //Allow no account request
       ) {
        QSharedPointer<EAiPrompt> aiPrompt;
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
        case DocumentSummary:
            aiPrompt.reset(new EAiDocSummaryPrompt(param, QString("")));
            aiCallback->setOp(DocumentSummary);
            break;
        default:
            qCWarning(logAIGUI) << "Unimplemented operation:" << act;
            break;
        }

        //Set ai model type
        if (!aiPrompt.isNull()) {
            aiPrompt->setLLM(llmType);
            reqId = EAiExec()->sendAiRequst(llmId, aiPrompt, aiCallback);
            qCDebug(logAIGUI) << "AI request sent - ID:" << reqId << "Type:" << act << "Mode:" << mode;
        }
    } else {
        qCWarning(logAIGUI) << "Invalid request parameters - llmId:" << llmId
                           << "act:" << act
                           << "mode:" << mode
                           << "param empty:" << param.isEmpty();
    }

    return reqId;
}

QString EAiProxy::sendRequest(const QString &llmId, const QString &chatChunkData)
{
    uos_ai::ChatChunk chatChunk;
    uos_ai::ChatChunk::json2ChatChunk(chatChunkData.toUtf8(), chatChunk);
    EAiExec()->sendRequest(llmId, chatChunk, this, getNotifierName(None, Mode::StreamMode));

    QJsonObject chatChunkObj;
    uos_ai::ChatChunk::chatChunk2Json(chatChunkObj, chatChunk);
    return QJsonDocument(chatChunkObj).toJson(QJsonDocument::Compact);
}

void EAiProxy::cancelAiRequest(const QString &id)
{
    qCDebug(logAIGUI) << "Cancelling AI request:" << id;
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

QString EAiProxy::currentAssistantId()
{
    return EAiExec()->currentAssistantId();
}

QString EAiProxy::queryAssistantList()
{
    return EAiExec()->queryAssistantList();
}

bool EAiProxy::setCurrentAssistantId(const QString &id)
{
    return EAiExec()->setCurrentAssistantId(id);
}

QString EAiProxy::getAiFAQ()
{
    return EAiExec()->getRandomAiFAQ();
}

QString EAiProxy::getAiFAQByFunction(int type, const QString &function)
{
    return EAiExec()->getRandomAiFAQByFunction(type, function);
}

QString EAiProxy::getAssistantFunctions(int type)
{
    return EAiExec()->getAssistantFunctions(type);
}

QString EAiProxy::getFunctionTemplate(int type, const QString &function, const QString &contain)
{
    return EAiExec()->getFunctionTemplate(type , function, contain);
}

void EAiProxy::launchLLMConfigWindow(bool showAddllmPage)
{
    QString locateTitle = "";
    if (!showAddllmPage)
        locateTitle = tr("Model Configuration");

    return EAiExec()->launchLLMConfigWindow(showAddllmPage, false, false, locateTitle);
}

void EAiProxy::launchLLMConfigWindowAndGetFreeDialog()
{
    return EAiExec()->launchLLMConfigWindowAndGetFreeDialog();
}

bool EAiProxy::getFreeCredits(bool isShowDlg)
{
    return EAiExec()->getFreeCredits(isShowDlg);
}

QString EAiProxy::getCurrentShortcut()
{
    return EAiExec()->getCurrentShortcut();
}

bool EAiProxy::isEnableAdvancedCssFeatures()
{
    #ifdef COMPILE_ON_QT6
        return true;
    #else
        return false;
    #endif
}

bool EAiProxy::isSimplifiedChinese()
{
    return EAiExec()->isSimplifiedChinese();
}

bool EAiProxy::isDeleteOutlineTitle()
{
    return EAiExec()->isDeleteOutlineTitle();
}

QString EAiProxy::getDownloadListIcon(const QString &fileSuffix)
{
    return EAiExec()->getDownloadListIcon(fileSuffix);
}

bool EAiProxy::downloadFile(const QString &id, const QString &title, const QString &content, const QString &suffix)
{
    return EAiExec()->downloadFile(id, title, content, suffix);
}

void EAiProxy::printDocument(const QString &html, const QString &title)
{
    return EAiExec()->printDocument(html, title);
}

void EAiProxy::updateAnswresActiveIndex(int activeIndex)
{
    EAiExec()->updateAnswresActiveIndex(activeIndex);
}

void EAiProxy::setDigitalImageDisable(bool disable)
{
    EAiExec()->setDigitalImageDisable(disable);
}

bool EAiProxy::isActiveChatFromDigitalImage()
{
    return EAiExec()->isActiveChatFromDigitalImage();
}

void EAiProxy::confirmDataSaved()
{
    qCDebug(logAIGUI) << "Frontend confirmed data saved, notifying SystemEventWatcher";
    SystemEventWatcher::instance().confirmDataSaved();
}

void EAiProxy::launchKnowledgeBaseConfigWindow()
{
    return EAiExec()->launchKnowledgeBaseConfigWindow();
}

void EAiProxy::launchMcpConfigWindow()
{
    return EAiExec()->launchMcpConfigWindow();
}

void EAiProxy::launchAboutWindow()
{
    return EAiExec()->launchAboutWindow();
}

void EAiProxy::setFontInfo(const QString &fontFamily, int pixelSize)
{
    m_fontFamily = fontFamily;
    m_fontPixelSize = pixelSize;
}

QString EAiProxy::fontInfo()
{
    return m_fontFamily + "#" + QString::number(m_fontPixelSize);
}

void EAiProxy::setWindowMode(bool isWindowMode)
{
    m_isWindowMode = isWindowMode;
}

bool EAiProxy::isWindowMode()
{
    return m_isWindowMode;
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

bool EAiProxy::startRecorder(int mode)
{
    qCDebug(logAIGUI) << "Starting audio recorder - mode:" << mode;
    m_audioLenLimit = AUDIO_LENGTH;

    //Stop time anyway.
    m_audioRecCounter->stop();
    m_audioRecCounter->start(1000);

    return EAiExec()->startRecorder(mode);
}

bool EAiProxy::stopRecorder()
{
    qCDebug(logAIGUI) << "Stopping audio recorder";
    m_audioRecCounter->stop();

    return EAiExec()->stopRecorder();
}

bool EAiProxy::playTextAudio(const QString &id, const QString &text, bool isEnd, bool isPlayOutline)
{
    return EAiExec()->playTextAudio(id, text, isEnd, isPlayOutline);
}

bool EAiProxy::stopPlayTextAudio()
{
    return EAiExec()->stopPlayTextAudio();
}

void EAiProxy::playSystemSound(int effId)
{
    return EAiExec()->playSystemSound(effId);
}

void EAiProxy::copyReplyText(const QString &reply)
{
    qCDebug(logAIGUI) << "Copying reply text to clipboard";
    if (ESystemContext::isTreeland()) {
        QTimer::singleShot(10, this, [reply]{
            QClipboard *clip = QApplication::clipboard();
            clip->setText(reply);
        });
        return;
    }

    QClipboard *clip = QApplication::clipboard();
    clip->setText(reply);
}

void EAiProxy::logAiChatRecord(const QString &reqId,
                               const QString &question,
                               const QString &anwser,
                               bool isRetry,
                               int err,
                               const QString &errorInfo,
                               int actionType,
                               const QString &llmIcon,
                               const QString &llmName,
                               const QString &docSummaryParam)
{
    qCDebug(logAIGUI) << "Logging chat record - ID:" << reqId
                      << "Action:" << actionType
                      << "Error:" << err;
    return EAiExec()->logChatRecord(
               reqId, question, QStringList(anwser),
               isRetry, err, errorInfo,
               actionType,
               llmIcon, llmName, docSummaryParam);
}

void EAiProxy::logAiChatRecord(const QString &reqId,
                               const QString &question,
                               const QStringList &anwser,
                               bool isRetry,
                               int err,
                               const QString &errorInfo,
                               int actionType,
                               const QString &llmIcon,
                               const QString &llmName,
                               const QString &docSummaryParam)
{
    return EAiExec()->logChatRecord(
               reqId, question, anwser,
               isRetry, err, errorInfo,
               actionType,
               llmIcon, llmName, docSummaryParam);
}

QString EAiProxy::getAiChatRecords(bool lastRec)
{
    return EAiExec()->getChatRecords(lastRec);
}

QString EAiProxy::getConversations()
{
    return EAiExec()->getConversations();
}

QString EAiProxy::createNewConversation()
{
    return EAiExec()->createNewConversation();
}

void EAiProxy::removeConversation(const QString &assistantId, const QString &conversationId)
{
    return EAiExec()->removeConversation(assistantId, conversationId);
}

void EAiProxy::removeAllConversation()
{
    return EAiExec()->removeAllConversation();
}

QString EAiProxy::getConversationHistoryList()
{
    return EAiExec()->getConversationHistoryList();
}

bool EAiProxy::setCurrentConversationId(const QString &assistantId, const QString &conversationId)
{
    return EAiExec()->setCurrentConversationId(assistantId, conversationId);
}

QString EAiProxy::getLastConversation(const QString &assistantId)
{
    return EAiExec()->getLastConversation(assistantId);
}

void EAiProxy::clearAiChatRecords()
{
    qCDebug(logAIGUI) << "Clearing all AI chat records";
    return EAiExec()->clearChatRecords();
}

void EAiProxy::logCurrentConversations(const QString &assistantId, const QString &conversationId, const QString &assistantDisplayName, const QString &conversationsData)
{
    QVector<uos_ai::Conversations> convs;
    uos_ai::Conversations::json2Convs(convs, conversationsData.toUtf8());

    EAiExec()->logConversations(assistantId, conversationId, assistantDisplayName, convs);
}

QString EAiProxy::saveImageAs(const QString &imageData, bool saveAs)
{
    return EAiExec()->saveImageAs(imageData, saveAs);
}

bool EAiProxy::previewImage(const QString &imageData)
{
    return EAiExec()->previewImage(imageData);
}

bool EAiProxy::previewImageForPath(const QString &imagePath)
{
    return EAiExec()->previewImageForPath(imagePath);
}

void EAiProxy::copyImage2Clipboard(const QString &filePath)
{
    return EAiExec()->copyImg2Clipboard(filePath);
}

bool EAiProxy::isNetworkAvailable()
{
    bool available = EAiExec()->isNetworkAvailable();
    qCDebug(logAIGUI) << "Network status:" << available;
    return available;
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
    translations["Click on the animation or Ctrl+Super+Space to activate"] = tr("Click on the animation or Ctrl+Super+Space to activate");
    translations["Voice input is temporarily unavailable, please check the network!"] = tr("Voice input is temporarily unavailable, please check the network!");
    translations["Unable to connect to the server, please check your network or try again later."] = tr("Unable to connect to the server, please check your network or try again later.");
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
    translations["Mode"] = tr("Mode");
    translations["Window Mode"] = tr("Window Mode");
    translations["Sidebar Mode"] = tr("Sidebar Mode");
    translations["Assistant List"] = tr("Assistant List");
    translations["Agent List"] = tr("Agent List");
    translations["Agent Store"] = tr("Agent Store");
    translations["UOS System Assistant"] = tr("UOS System Assistant");
    translations["Deepin System Assistant"] = tr("Deepin System Assistant");
    translations["Personal Knowledge Assistant"] = tr("Personal Knowledge Assistant");
    translations["Please configure the knowledge base"] = tr("Please configure the knowledge base");
    translations["knowledge base configure content"] = tr("Before using the [Personal Knowledge Assistant], it is necessary to configure the knowledge base. After configuring the knowledge base, AI will answer questions or generate content based on the content you have configured in the knowledge base.");
    translations["Please configure the large model"] = tr("Please configure the large model");
    translations["The personal knowledge assistant can only be used after configuring a large model."] = tr("The personal knowledge assistant can only be used after configuring a large model.");
    translations["To configure"] = tr("To configure");
    translations["To install"] = tr("To install");
    translations["Please install EmbeddingPlugins"] = tr("Please install [EmbeddingPlugins]");
    translations["EmbeddingPlugins install content"] = tr("This assistant requires the installation of the EmbeddingPlugins to run");

    // document summary
    translations["Drag files here to add them."] = tr("Drag files here to add them.");
    translations["You can only add 3 files, supported file types include: txt, doc, docx, xls, xlsx, ppt, pptx, pdf, md, png, jpg, jpeg, code files, etc."] = tr("You can only add 3 files, supported file types include: txt, doc, docx, xls, xlsx, ppt, pptx, pdf, md, png, jpg, jpeg, code files, etc.");
    translations["You can only add a maximum of one file."] = tr("You can only add a maximum of one file.");
    translations["The file format is not supported."] = tr("The file format is not supported.");
    translations["Summarize the key content of the file."] = tr("Summarize the key content of the file.");
    translations["Parsing..."] = tr("Parsing...");
    translations["File Error"] = tr("File Error");
    translations["File has been deleted."] = tr("File has been deleted.");
    translations["The file size exceeds the 100MB limit."] = tr("The file size exceeds the 100MB limit.");
    translations["Upload a document"] = tr("Upload a document");
    translations["File deleted"] = tr("File deleted");
    translations["No text was parsed"] = tr("No text was parsed");
    translations["Reference"] = tr("Reference");

    // Instruction
    translations["Instruction"] = tr("Instruction");
    translations["Type \"/\" in the input box to activate."] = tr("Type \"/\" in the input box to activate.");
    translations["Please enter; “Ctrl+Enter” to change the line."] = tr("Please enter; “Ctrl+Enter” to change the line.");
    translations["Enter your question, or enter \"/\" to select a command\n\"Ctrl+Enter\"  to start a new line"] = tr("Enter your question, or enter \"/\" to select a command\n\"Ctrl+Enter\"  to start a new line");

    //联网搜索
    translations["Search complete."] = tr("Search complete.");
    translations["Click to view results"] = tr("Click to view results");

    // textToPicture
    translations["edit"] = tr("edit");
    translations["save"] = tr("save");
    translations["copy"] = tr("copy");

    // 2.6需求
    translations["Search"] = tr("Search");
    translations["DeepThink"] = tr("DeepThink");
    translations["Thinking has stopped"] = tr("Thinking has stopped");
    translations["Back to bottom"] = tr("Back to bottom");
    translations["Deeply thought (%1 seconds)"] = tr("Deeply thought (%1 seconds)");

    // 2.7需求
    translations["New Conversation"] = tr("New Conversation");
    translations["History"] = tr("History");
    translations["No History Records"] = tr("No History Records");
    translations["Today"] = tr("Today");
    translations["Yesterday"] = tr("Yesterday");
    translations["Are you sure to delete the conversation? It will be unrecoverable once deleted."] = tr("Are you sure to delete the conversation? It will be unrecoverable once deleted.");
    translations["The %1 agent used in this conversation has been deleted"] = tr("The %1 agent used in this conversation has been deleted");
    translations["This conversation cannot be viewed. To view it, please install the %1 agent and try again."] = tr("This conversation cannot be viewed. To view it, please install the %1 agent and try again.");
    translations["The original conversation model has been deleted. We have switched to a new model for you to continue the conversation."] = tr("The original conversation model has been deleted. We have switched to a new model for you to continue the conversation.");

    // 2.8需求
    translations["Recommendations"] = tr("Recommendations");
    translations["More"] = tr("More");
    translations["Add Model"] = tr("Add Model");
    translations["No Model"] = tr("No Model");
    translations["No model available. Please install or configure a model in the settings."] = tr("No model available. Please install or configure a model in the settings.");
    translations["Please Describe the Content Theme and Requirements for Your Creation."] = tr("Please Describe the Content Theme and Requirements for Your Creation.");
    translations["Please Enter the Content You Want to Translate and Specify the Target Language. Default Translation is to Chinese."] = tr("Please Enter the Content You Want to Translate and Specify the Target Language. Default Translation is to Chinese.");
    translations["Please Enter the Text You Need to Process and Specify Your Requirements."] = tr("Please Enter the Text You Need to Process and Specify Your Requirements.");
    translations["New Agent Added"] = tr("New Agent Added");
    translations["New Writing, Text Processing, and Translation Agents have been added. Check them out now."] = tr("New Writing, Text Processing, and Translation Agents have been added. Check them out now.");
    translations["Try it"] = tr("Try it");
    translations["Write an article based on the following document:"] = tr("Write an article based on the following document:");
    translations["Translate the following document into English:"] = tr("Translate the following document into English:");

    // 2.9需求
    translations["MCP Server"] = tr("MCP Server");
    translations["Add Mcp Server"] = tr("Add Mcp Server");  //添加MCP服务器
    translations["Add Server"] = tr("Add Server");
    translations["Add failed! Error reason:"] = tr("Add failed! Error reason:");
    translations["MCP & Skills environment missing. Please install 【UOS AI Agent】"] = tr("MCP & Skills environment missing. Please install 【UOS AI Agent】");
    translations["Calling"] = tr("Calling");
    translations["Completed"] = tr("Completed");
    translations["Call Failed"] = tr("Call Failed");
    translations["Cancelled"] = tr("Cancelled");
    translations["params"] = tr("params");
    translations["result"] = tr("result");
    translations["For MCP & Skills Server, it is recommended to switch to the official model \"Intelligent Routing\""] = tr("For MCP & Skills Server, it is recommended to switch to the official model \"Intelligent Routing\"");
    translations["Enter MCP & Skills Server command, e.g., \"Change system to dark mode for me\""] = tr("Enter MCP & Skills Server command, e.g., \"Change system to dark mode for me\"");
    translations["Agent server is not available"] = tr("Agent server is not available");  //智能体服务不可用 11000
    translations["Agent server exception"] = tr("Agent server exception");  //智能体服务异常 11001
    translations["MCP server is not available"] = tr("MCP server is not available");  //MCP服务不可用 11100
    translations["Cancel"] = tr("Cancel");
    translations["Confirm"] = tr("Confirm");
    translations["Automate multi-file and multi-app tasks with one command using MCP Service. Try it now!"] = tr("Automate multi-file and multi-app tasks with one command using MCP Service. Try it now!");
    translations["Use later"] = tr("Use later");
    translations["Install Now"] = tr("Install Now");
    translations["Enable MCP Server"] = tr("Enable MCP Server");
    translations["After installing the MCP environment \"UOS AI Agent\", click the "] = tr("After installing the MCP environment \"UOS AI Agent\", click the ");
    translations[" and select \"uos-mcp\" in the MCP server list."] = tr(" and select \"uos-mcp\" in the MCP server list.");
    translations["Try saying: \"Change system to dark mode\"."] = tr("Try saying: \"Change system to dark mode\".");
    translations["Try it now"] = tr("Try it now");
    translations["Add Mcp Server[GuidePage]"] = tr("Add Mcp Server[GuidePage]");  //新增MCP服务
    translations["First-time users: Install MCP environment \"UOS AI Agent\" via App Store."] = tr("First-time users: Install MCP environment \"UOS AI Agent\" via App Store.");
    translations["The JSON file format is incorrect, please check and submit again"] = tr("The JSON file format is incorrect, please check and submit again");
    translations["Install Now >"] = tr("Install Now >");  // 去安装

    // 2.10需求
    translations["General Chat"] = tr("General Chat");  // 普通对话
    translations["Private Chat"] = tr("Private Chat");  // 隐私对话
    translations["Now in Private Chat"] = tr("Now in Private Chat");  // 已经进入隐私对话
    translations["Private Chat messages are not saved in history and will be permanently deleted when you leave the chat."] = tr("Private Chat messages are not saved in history and will be permanently deleted when you leave the chat.");  // 隐私对话不会显示在历史记录中，离开对话界面时，其内容会被完全删除。
    translations["Screenshot Q&A    Shortcut (Ctrl+Alt+Q), up to 3 images supported."] = tr("Screenshot Q&A    Shortcut (Ctrl+Alt+Q), up to 3 images supported.");  // 截图问答 快捷键（Ctrl+Alt+Q），最多支持 3 张图
    translations["Cannot be used during screen recording"] = tr("Cannot be used during screen recording");  // 截图问答 不能在屏幕录制中使用
    translations["You can upload up to 3 files or image"] = tr("You can upload up to 3 files or image");  // 文件和图片的总数最多为3个
    translations["Upload Files"] = tr("Upload Files");  // 上传文件
    translations["Please delete the abnormal file and send it again"] = tr("Please delete the abnormal file and send it again");  // 请删除异常文件再发送。
    translations["Add Private Chat"] = tr("Add Private Chat");  // 添加隐私对话
    translations["Add [Screenshot Q&A]"] = tr("Add [Screenshot Q&A]");  // 添加截图问答
    translations["Take a screenshot and send the content to UOS AI. You can also upload an image directly."] = tr("Take a screenshot and send the content to UOS AI. You can also upload an image directly.");
    translations["OK"] = tr("OK");
    translations["Next"] = tr("Next");
    translations["Add [ Private Chat Mode ] - Chats will not be saved."] = tr("Add [ Private Chat Mode ] - Chats will not be saved.");  // 添加隐私对话
    translations["No text extracted"] = tr("No text extracted");  // 未提取到文字
    translations["Image size exceeds 15 MB"] = tr("Image size exceeds 15 MB");  // 文件大小超过15 MB

    // 2.11需求
    translations["After opening the knowledge base, answers will be based on its content. Response speed depends on machine performance and the size of the knowledge base."] = tr("After opening the knowledge base, answers will be based on its content. Response speed depends on machine performance and the size of the knowledge base.");  // 打开知识库后，会基于知识库回答问题。回答速度受机器性能和知识库数量影响。
    translations["Knowledge base unavailable when any command or MCP is selected."] = tr("Knowledge base unavailable when any command or MCP is selected.");  // 在选中任意指令或MCP时，知识库不可用。
    translations["MCP is disabled while the knowledge base is active."] = tr("MCP is disabled while the knowledge base is active.");  // 在使用知识库时，MCP功能不可用。
    translations["Commands disabled while knowledge base is active."] = tr("Commands disabled while knowledge base is active.");  // 在使用知识库时，指令功能不可用。
    translations["Copy"] = tr("Copy");  // 复制
    translations["Re-edit"] = tr("Re-edit");  // 重新编辑
    translations["Copy succeeded."] = tr("Copy succeeded.");  // 复制成功。
    translations["Copy failed. Please try again."] = tr("Copy failed. Please try again.");  // 复制失败. 请重试。
    translations["Searching"] = tr("Searching");  // 搜索中
    translations["%1 reference documents have been obtained (%2s)"] = tr("%1 reference documents have been obtained (%2s)");  // 搜索到%1个参考文档（用时%2秒）
    translations["Clear History"] = tr("Clear History");  // 清除历史记录
    translations["Delete all records?"] = tr("Delete all records?");  // 是否要删除全部记录？
    translations["Once deleted, the content cannot be recovered!"] = tr("Once deleted, the content cannot be recovered!");  // 删除后，内容将无法恢复！
    translations["Recommend official models"] = tr("Recommend official models");
    translations["Disable MCP"] = tr("Disable MCP");

    // 2.12需求
    translations["It is recommended to use the official model \"Intelligent Routing\""] = tr("It is recommended to use the official model \"Intelligent Routing\"");  // 推荐使用官方模型"智能调度"
    translations["Quick Open"] = tr("Quick Open");  // 快速唤起
    translations["MCP Server Upgrade to Automatic Mode"] = tr("MCP Server Upgrade to Automatic Mode");  // MCP服务升级自动模式
    translations["MCP Server have been upgraded to automatic mode, allowing you to access all MCP Server with just click "] = tr("MCP Server have been upgraded to automatic mode, allowing you to access all MCP Server with just click ");  //MCP服务升级自动模式，仅需打开  
    translations[". This allows you to automate tasks like system setup and file processing with just one click."] = tr(". This allows you to automate tasks like system setup and file processing with just one click.");   //按钮即可使用所有MCP服务。您可以一句话完成系统设置、文件处理等自动化任务。
    translations["Adding MCP Server has been moved to Settings."] = tr("Adding MCP Server has been moved to Settings.");  // 添加MCP服务移至设置
    translations["To add more MCP Server, go to Settings > MCP Server."] = tr("To add more MCP Server, go to Settings > MCP Server.");  // 如需添加更多MCP服务，请到"设置-MCP服务"中添加。
    translations["Got it"] = tr("Got it");  // 知道了
    translations["Complimentary Model Credits"] = tr("Complimentary Model Credits");  //  【福利】模型额度赠送
    translations["The current system offers the \"Intelligent Routing\" model, which automatically refreshes your free quota at the beginning of each month, allowing you to use it worry-free."] = tr("The current system offers the \"Intelligent Routing\" model, which automatically refreshes your free quota at the beginning of each month, allowing you to use it worry-free.");  // 当前系统赠送的"智能调度"模型，每月初自动刷新免费额度，让您畅用无忧。
    translations["Claim Credits"] = tr("Claim Credits");  // 领取额度
    translations["Get a free account"] = tr("Get a free account");  // 领取免费账号
    translations["Claim Free Credits"] = tr("Claim Free Credits");  // 领取免费额度
    translations["Successfully Claimed"] = tr("Successfully Claimed");  // 领取成功
    translations["Failed to Claim. Please Try Again."] = tr("Failed to Claim. Please Try Again.");  // 领取失败，请再试一次
    translations["Enable MCP Server&"] = tr("Enable MCP Server&");  // 开启MCP服务
    translations["Disable MCP Server"] = tr("Disable MCP Server");  // 关闭MCP服务
    translations["Configure MCP Server"] = tr("Configure MCP Server");  // 配置MCP服务

    // UOS AI写作需求
    translations["Collecting and analyzing data"] = tr("Collecting and analyzing data");  // 正在搜集和分析资料
    translations["Data collection and analysis completed"] = tr("Data collection and analysis completed");  // 已完成资料搜集和分析
    translations["Local Materials"] = tr("Local Materials");  // 本地素材
    translations["File Outline"] = tr("File Outline");  // 文件大纲
    translations["I am [Enter Identity/Position]. Please help me write a [Report/Article/Outline/WeChat Official Account Post/Notice/Research Report/Work Summary/Speech] on [Enter Theme], with a length of about [1000 words]. The content requirements are [Enter Requirements/Content Focus/Content Style, etc.]."] = tr("I am [Enter Identity/Position]. Please help me write a [Report/Article/Outline/WeChat Official Account Post/Notice/Research Report/Work Summary/Speech] on [Enter Theme], with a length of about [1000 words]. The content requirements are [Enter Requirements/Content Focus/Content Style, etc.].");
    translations["Supports uploading up to 10 local materials"] = tr("Supports uploading up to 10 local materials");  // 最多支持上传10个本地素材
    translations["Only supports uploading 1 outline file"] = tr("Only supports uploading 1 outline file");  // 仅支持上传1个大纲文件
    translations["Confirm deletion of this reference material?"] = tr("Confirm deletion of this reference material?");  // 确定删除该参考素材？
    translations["Confirm deletion of this outline file?"] = tr("Confirm deletion of this outline file?");  // 确定删除该大纲文件？
    translations["Outline:"] = tr("Outline:");  // 大纲
    translations["Saving..."] = tr("Saving...");  // 保存中...
    translations["Saved successfully!"] = tr("Saved successfully!");  // 保存成功！
    translations["Save failed, please try again!"] = tr("Save failed, please try again!");  // 保存失败，请再试一次
    translations["Text document"] = tr("Text document");  // 文本文档
    translations["You can continue to input more requests to optimize or adjust the already generated content."] = tr("You can continue to input more requests to optimize or adjust the already generated content.");  // 您还可以继续输入更多要求，对已生成的内容优化或调整
    translations["AI Writing Agent Fully Upgraded"] = tr("AI Writing Agent Fully Upgraded");  // AI写作智能体全新升级
    translations["1.Reference local materials and outlines for more accurate content."] = tr("1.Reference local materials and outlines for more accurate content.");  // 1.参考本地资料和大纲，内容更准
    translations["2.Supports local models, ensuring security and peace of mind."] = tr("2.Supports local models, ensuring security and peace of mind.");  // 2.支持本地模型，安全无忧
    translations["3.Traceable sources, reliable data."] = tr("3.Traceable sources, reliable data.");  // 3.来源可查，数据可靠
    translations["4.Edit while writing, export when satisfied."] = tr("4.Edit while writing, export when satisfied.");  // 4.边写边改，满意导出
    translations["Try Later"] = tr("Try Later");  // 稍后再试
    translations["Try Now"] = tr("Try Now");  // 前往体验
    translations["Generate reports based or outline files for greater accuracy"] = tr("Generate reports based or outline files for greater accuracy");  // 可基于上传的本地素材或大纲文件来生成报告，内容更准确
    translations["Start Now"] = tr("Start Now");  // 开始体验

    // 大纲组件
    translations["Enter Chapter Title"] = tr("Enter Chapter Title");  // 输入章节标题
    translations["Add Chapter"] = tr("Add Chapter");  // 增加章节
    translations["Add Section"] = tr("Add Section");  // 增加子章节
    translations["Delete Chapter"] = tr("Delete Chapter");  // 删除章节
    translations["Outline to Docs"] = tr("Outline to Docs");  // 基于大纲生成文档
    translations["Save as Word"] = tr("Save as Word");  // 另存为Word
    translations["Save as PDF"] = tr("Save as PDF");  // 另存为PDF
    translations["Save as Markdown"] = tr("Save as Markdown");  // 另存为Markdown
    translations["Content generation completed, but you can still provide revision suggestions in AI Assistant"] = tr("Content generation completed, but you can still provide revision suggestions in AI Assistant");  // 内容生成完毕，但你仍可在AI助手中继续提出修改意见
    translations["Please input the title"] = tr("Please input the title");  // 请输入标题
    translations["Bold"] = tr("Bold");
    translations["Copied successfully"] = tr("Copied successfully");
    translations["Save As"] = tr("Save As");
    translations["Saving..."] = tr("Saving...");
    translations["Saved successfully!"] = tr("Saved successfully!");
    translations["Undo"] = tr("Undo");
    translations["Redo"] = tr("Redo");
    translations["Return to conversation"] = tr("Return to conversation");
    translations["Print"] = tr("Print");
    translations["Saving As..."] = tr("Saving As...");  // 另存中...
    translations["Save As successful!"] = tr("Save As successful!");  // 另存成功！
    translations["Save As failed. Please retry."] = tr("Save As failed. Please retry.");  // 另存失败，请再试一次
    translations["Body text"] = tr("Body text");  // 正文
    translations["Heading 1"] = tr("Heading 1");  // 标题1
    translations["Heading 2"] = tr("Heading 2");  // 标题2
    translations["Heading 3"] = tr("Heading 3");  // 标题3
    translations["Heading 4"] = tr("Heading 4");  // 标题4
    translations["Heading 5"] = tr("Heading 5");  // 标题5
    translations["Heading 6"] = tr("Heading 6");  // 标题6
    translations["Unordered list"] = tr("Unordered list");  // 无序列表
    translations["Ordered list"] = tr("Ordered list");  // 有序列表
    translations["Digital Human Unavailable"] = tr("Digital Human Unavailable");  // 数字人暂不可用

    // skills
    translations["Enable MCP & Skills"] = tr("Enable MCP & Skills");  // 开启 MCP & Skills
    translations["Disable MCP & Skills"] = tr("Disable MCP & Skills");  // 关闭 MCP & Skills
    translations["Configure MCP & Skills"] = tr("Configure MCP & Skills");  // 配置 MCP & Skills

    return translations;
}
