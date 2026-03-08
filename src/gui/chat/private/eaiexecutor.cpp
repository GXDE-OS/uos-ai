#include "eaiexecutor.h"
#include "eaistreamhandler.h"
#include "serverwrapper.h"
#include "audiocontroler.h"
#include "application.h"
#include "networkmonitor.h"
#include "utils/util.h"
#include "embeddingserver.h"
#include "serverdefs.h"
#include "dbwrapper.h"
#include "localmodelserver.h"
#include "private/welcomedialog.h"
#include "private/easyncworker.h"
#include "eparserdocument.h"
#include "eappaiprompt.h"
#include "llm/common/llmutils.h"
#include "../Instruction/instructionmanager.h"
#include "eaichatinfojsoncontrol.h"
#include "agentfactory.h"
#include "mcpserver.h"
#include "mcpconfigsyncer.h"
#include "dconfigmanager.h"
#include "global_define.h"

#include <report/assistantchatpoint.h>
#include <report/digitalchatpoint.h>
#include <report/privatechatpoint.h>
#include <report/privatechatclickedpoint.h>
#include <report/assistantchattypepoint.h>
#include <report/modelpoint.h>
#include <report/eventlogutil.h>

#include <QDBusConnection>
#include <QDBusReply>
#include <QRegularExpression>
#include <QApplication>
#include <QFile>
#include <QDateTime>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QTimer>
#include <QUuid>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QClipboard>
#include <QLoggingCategory>

#include <DFileDialog>
#include <docparser.h>

#include <chrono>

//Comment the following line
#define AI_DEBUG

UOSAI_USE_NAMESPACE

//Qt::SkipEmptyParts was introduced
// or modified in Qt 5.14.
#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
#define PARAM_SKIP_EMPTY QString::SkipEmptyParts
#else
#define  PARAM_SKIP_EMPTY Qt::SkipEmptyParts
#endif

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

EAiExecutor::EAiExecutor(QObject *parent)
    : QObject(parent)
{
    initAiSession();

    initAudioRecord();

    initNetworkMonitor();

    loadAiFAQ();

    JudgeIsShowFreeAccountGuide();

    m_knowledgeBaseExist = EmbeddingServer::getInstance().getDocFiles().size();
    m_embeddingPluginsExist = LocalModelServer::getInstance().checkInstallStatus(PLUGINSNAME);

    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::addToServerStatusChanged, this, &EAiExecutor::onAddToServerStatusChanged);
    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::indexDeleted, this, &EAiExecutor::onIndexDeleted);
    connect(&LocalModelServer::getInstance(), &LocalModelServer::localLLMStatusChanged, this, &EAiExecutor::onLocalLLMStatusChanged);
    connect(&LocalModelServer::getInstance(), &LocalModelServer::modelPluginsStatusChanged, this, &EAiExecutor::onEmbeddingPluginsStatusChanged);

    connect(EParserDocument::instance(), &EParserDocument::sigParserStart, this, &EAiExecutor::docSummaryParsingStart);
    connect(EParserDocument::instance(), &EParserDocument::sigParserResult, this, &EAiExecutor::docDragInViewParserResult);
}

bool EAiExecutor::initAiSession()
{
    if (m_aiProxy.isNull()) {
        m_aiProxy = ServerWrapper::instance()->createChatSession();

        connect(m_aiProxy.get(), &Session::chatTextReceived,
                this, &EAiExecutor::onChatTextReceived);
        connect(m_aiProxy.get(), &Session::error,
                this, &EAiExecutor::onChatError);

        connect(m_aiProxy.get(), &Session::llmAccountLstChanged,
                this, &EAiExecutor::llmAccountLstChanged);
        connect(m_aiProxy.get(), &Session::uosAiLlmAccountLstChanged,
                this, &EAiExecutor::uosAiLlmAccountLstChanged);

        //Init TTP
        connect(m_aiProxy.get(), &Session::chatAction,
                this, &EAiExecutor::chatConversationType);
        connect(m_aiProxy.get(), &Session::text2ImageReceived
                , this, &EAiExecutor::onTextToPictureData);

        connect(m_aiProxy.get(), &Session::assistantListChanged
                , this, &EAiExecutor::sigAssistantListChanged);

        connect(m_aiProxy.get(), &Session::previewReference
                , this, &EAiExecutor::previewReference);

        connect(m_aiProxy.get(), &Session::sigClaimAccountUsageFinished
                , this, &EAiExecutor::onAccountUsageClaimed);

        connect(m_aiProxy.get(), &Session::sigClaimAgain
                , this, &EAiExecutor::sigClaimAgain);
    }

    return !m_aiProxy.isNull();
}

void EAiExecutor::sendRequest(const QString &llmId, ChatChunk &chatChunk, QObject *caller, const QString &notifier)
{
    QJsonArray extentions = QJsonDocument::fromJson(chatChunk.extention.toUtf8()).array();
    QString userQuestion = appendDocContent(extentions, chatChunk.displayContent);

    QSharedPointer<EAiPrompt> aiPrompt = initPrompt(userQuestion, extentions);
    QSharedPointer<EAiCallback> aiCallback = initCallback(caller, notifier, aiPrompt);
    QString reqId = processRequest(aiPrompt, aiCallback, llmId, chatChunk);

    chatChunk.content = aiPrompt->getAiPrompt();
    chatChunk.reqId = reqId;

    qCInfo(logAIGUI) << "Request processed successfully, reqId:" << reqId;
}

QString EAiExecutor::sendWordWizardRequest(const QString &llmId, const QString &prompt, QObject *caller, const QString &notifier)
{
    QSharedPointer<EAiCallback> aiCallback = initDefaultCallback(caller, notifier);

    QString aiCtx = makeReq(prompt); // 单轮对话，无上下文
    QString reqId = wordWizardRequest(llmId, aiCtx, aiCallback);

    // tid:1001600008 event:assistantchat
    ReportIns()->writeEvent(report::AssistantChatPoint("UOS AI").assemblingData());

    return reqId;
}

QString EAiExecutor::sendAiRequst(const QString &llmId, QSharedPointer<EAiPrompt> prompt, QSharedPointer<EAiCallback> callback, bool isFAQGeneration)
{
    QString reqId("");

#ifdef AI_DEBUG
    qCInfo(logAIGUI) << "send ai request llmId:" << llmId << " prompt:" << prompt->getUserParam();
#endif

    //Make ai context
    QString aiCtx = makeAiReqContext(prompt, isFAQGeneration);

    QPair<AIServer::ErrorType, QStringList> reply;
    if ((m_aiProxy->currentAssistantType() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT
         || m_aiProxy->currentAssistantType() == AssistantType::DEEPIN_SYSTEM_ASSISTANT
         || m_aiProxy->currentAssistantType() == AssistantType::UOS_SYSTEM_ASSISTANT) && (!m_knowledgeBaseExist || !m_embeddingPluginsExist)) {
        QString uuid = QUuid::createUuid().toString(QUuid::Id128);
        QStringList list;
        list << uuid;
        if (!m_knowledgeBaseExist && m_embeddingPluginsExist)
        {
            if(m_aiProxy->currentAssistantType() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT){
                list << tr("The Personal Knowledge Assistant can only be used after configuring the knowledge base.");
                reply = qMakePair(AIServer::PersonalBaseNotExist, list);
            }
            else if(m_aiProxy->currentAssistantType() == AssistantType::DEEPIN_SYSTEM_ASSISTANT
                    || m_aiProxy->currentAssistantType() == AssistantType::UOS_SYSTEM_ASSISTANT)
            {
                QVariantHash params = {
                    {PREDICT_PARAM_STREAM, QVariant(callback->isStreamMode())},
                };
                reply = m_aiProxy->requestChatText(llmId, aiCtx, params);
            }
        }
        else if (!m_embeddingPluginsExist){
            if(m_aiProxy->currentAssistantType() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT)
                list << tr("The Personal Knowledge Assistant can only be used after configuring the model plug.");
            if(m_aiProxy->currentAssistantType() == AssistantType::DEEPIN_SYSTEM_ASSISTANT)
                list << tr("The Deep System Assistant can only be used after configuring the model plug.");
            if(m_aiProxy->currentAssistantType() == AssistantType::UOS_SYSTEM_ASSISTANT)
                list << tr("The UOS System  Assistant can only be used after configuring the model plug.");
            reply = qMakePair(AIServer::PersonalBaseNotExist, list);
        }
    } else {
        QVariantHash params = {
            {PREDICT_PARAM_STREAM, QVariant(callback->isStreamMode())},
        };
        reply = m_aiProxy->requestChatText(llmId, aiCtx, params);
    }

    QStringList params(reply.second);
    reqId = params.value(0);
    if (isFAQGeneration)
        m_faqId = reqId;

    if (reply.first == AIServer::ErrorType::NoError) {
        //<call ID/Socket ID> <signal name>
        //We Only use the call ID
        //Call ID is same as scoket ID
        m_callQueue.insert(reqId, callback);

        if (callback->isStreamMode()) {
            //EAiStreamHandler will release itself after
            //all of data is readed
            EAiStreamHandler *aiStreamHandler = new EAiStreamHandler(
                        reqId, callback);
            aiStreamHandler->process();
        }

        if (!isFAQGeneration)
            m_isAnswering = true;
    } else {
        //Handle the no account error code
        int errCode = reply.first;
        QString errInfo(reply.second.value(1));

        //the notify should be delay after the request is return.
        QTimer::singleShot(30, this, [callback, errInfo, errCode] {
            QJsonObject message;
            QJsonObject content;
            content.insert("content", errInfo);
            content.insert("chatType", ChatAction::ChatTextPlain);
            message.insert("message", content);
            message.insert("stream", true);
            QString errString = QString(QJsonDocument(message).toJson());
            callback->notify(errString, -errCode);
        });

        qCInfo(logAIGUI) << "reqId:" << reqId
                << " error:" << errCode
                << " errorString:" << errInfo;

        if (isFAQGeneration)
            m_isFAQGenerating = false;
        else
            m_isAnswering = false;
    }

    if (!isFAQGeneration && m_chatWindow) {
        m_chatWindow->setVoiceConversationDisabled(m_isAnswering);
        m_chatWindow->setHasChatHistory(true);
    }
    return reqId;
}

void EAiExecutor::clearAiRequest(const QObject *aiProxy)
{
    auto reqiter = m_callQueue.begin();
    for (; reqiter != m_callQueue.end(); reqiter++) {
        if (!reqiter->isNull()
                && (reqiter->get()->getOwner() == aiProxy)) {
            qCInfo(logAIGUI) << "Stop request:"
                    << " aiProxy=" << aiProxy
                    << " callId=" << reqiter.key()
                    << " isStreamMode=" << reqiter->get()->isStreamMode();

            reqiter->get()->setOwner(nullptr);
        }
    }
}

void EAiExecutor::cancelAiRequst(const QString &id)
{
    qCInfo(logAIGUI) << "Cancel the request:" << id;

    //Notify copilot cancel the task
    m_aiProxy->cancelRequestTask(id);

    //Remove the task from the callQueue
    //Don't need send the reply data anymore.

    //cancelRequestTask Request will throw
    //cancel error, remove the task in the chatError
    //instead.

    auto reqIter = m_callQueue.find(id);

    if (reqIter != m_callQueue.end()) {
        m_callQueue.erase(reqIter);
    }

}

QString EAiExecutor::currentLLMAccountId()
{
    auto accountId = m_aiProxy->currentLLMAccountId();

    return accountId;
}

QString EAiExecutor::uosAiLLMAccountId() {
    auto accountId = m_aiProxy->uosAiLLMAccountId();

    return accountId;
}

QString EAiExecutor::queryLLMAccountList()
{
    auto accountInfos = m_aiProxy->queryLLMAccountListWithRole();

    return accountInfos;
}

QString EAiExecutor::queryUosAiLLMAccountList() {
    auto accountInfos = m_aiProxy->queryUosAiLLMAccountList();

    return accountInfos;
}

bool EAiExecutor::setCurrentLLMAccountId(const QString &id)
{
    bool fSetOk = m_aiProxy->setCurrentLLMAccountId(id);

    return fSetOk;
}

bool EAiExecutor::setUosAiLLMAccountId(const QString &id) {
    bool fSetOk = m_aiProxy->setUosAiLLMAccountId(id);

    return fSetOk;
}

QString EAiExecutor::currentAssistantId()
{
    return m_aiProxy->currentAssistantId();
}

QString EAiExecutor::queryAssistantList()
{
    auto assistantInfos = m_aiProxy->queryAssistantList();
    return assistantInfos;
}

bool EAiExecutor::setCurrentAssistantId(const QString &id)
{
    qCInfo(logAIGUI) << "Setting current assistant ID:" << id;
    bool fSetOk = m_aiProxy->setCurrentAssistantId(id);

    if (id == "PPT Assistant" || id == "Poster Assistant") {
        m_chatWindow->setChatButtonVisible(false);
    } else {
        m_chatWindow->setChatButtonVisible(true);
    }

    return fSetOk;
}

bool EAiExecutor::setCurrentConversationId(const QString &assistantId, const QString &conversationId)
{
    qCInfo(logAIGUI) << "Setting conversation:"
                     << "assistant=" << assistantId
                     << "conversation=" << conversationId;
    if (m_conversionMode == ConversionMode::Private) {
        m_chatWindow->setHasChatHistory(m_currnetPrivateConvs.length() > 0);
        return true;
    }
    bool fSetOk = true;
    if (assistantId != m_aiProxy->currentAssistantId()) {
        fSetOk = m_aiProxy->setCurrentAssistantId(assistantId);
        if (assistantId == "PPT Assistant" || assistantId == "Poster Assistant") {
            m_chatWindow->setChatButtonVisible(false);
        } else {
            m_chatWindow->setChatButtonVisible(true);
        }
    }
    if (fSetOk) {
        fSetOk = EaiChatInfoJsonControl::localChatInfoJsonControl().isConvsExist(assistantId, conversationId);
    }
    if (fSetOk) {
        m_currnetConvs.clear();
        fSetOk = EaiChatInfoJsonControl::localChatInfoJsonControl().getConvsInfo(assistantId, conversationId, m_currnetConvs);
        m_chatWindow->setHasChatHistory(m_currnetConvs.size() > 0);
    }

    return fSetOk;
}

QString EAiExecutor::getLastConversation(const QString &assistantId)
{
    return EaiChatInfoJsonControl::localChatInfoJsonControl().findLatestConversation(assistantId);
}

QString EAiExecutor::currentAssistantName()
{
    return m_aiProxy->currentAssistantDisplayName();
}

AssistantType EAiExecutor::currentAssistantType()
{
    return m_aiProxy->currentAssistantType();
}

void EAiExecutor::launchLLMConfigWindow(bool showAddllmPage, bool onlyUseAgreement, bool isFromAiQuick, const QString &locateTitle)
{
    m_aiProxy->launchLLMUiPage(showAddllmPage, onlyUseAgreement, isFromAiQuick, locateTitle);
}

void EAiExecutor::launchLLMConfigWindowAndGetFreeDialog()
{
    m_aiProxy->launchGetFreeAccountDlg();
}

void EAiExecutor::launchKnowledgeBaseConfigWindow()
{
    QString locateTitle(tr("Knowledge Base Management"));
    m_aiProxy->launchKnowledgeBaseUiPage(locateTitle);
}

void EAiExecutor::launchMcpConfigWindow()
{
    QString locateTitle(tr("MCP Server"));
    m_aiProxy->launchKnowledgeBaseUiPage(locateTitle);
}

void EAiExecutor::launchAboutWindow()
{
    m_aiProxy->launchAboutWindow();
}

QString EAiExecutor::getRandomAiFAQ()
{
    QJsonArray faqArray;
    //Get a random position
    QRandomGenerator randomGenerator(QDateTime::currentSecsSinceEpoch());
    if (!m_isFAQGenerating && !currentLLMAccountId().isEmpty() && m_knowledgeBaseExist && m_persKnowledgeBaseFAQ.isEmpty()) {
        personalFAQGenerate();
    }

    switch (m_aiProxy->currentAssistantType()) {
    case UOS_AI: {
        int endPosition = 7;
        switch (LLMServerProxy::llmManufacturer(m_aiProxy->currentLLMModel())) {
        case ModelManufacturer::CHATGPT:
        case ModelManufacturer::BAIDU_WXQF:
            if (m_aiProxy->currentLLMAccountId().isEmpty())
                break;

            if (m_aiProxy->currentModelType() == ModelType::FREE_NORMAL) {
                // 只有百度的免费账号才显示文生图提问
                endPosition = 17;
            } else {
                endPosition = 16;
            }
            break;
        default:
            break;
        }

        QList<int> numbers;
        QSet<int> uniqueSet;

        while (numbers.size() < m_limitAQCount) {
            int randomNum = QRandomGenerator::global()->bounded(0, endPosition);
            if (uniqueSet.contains(randomNum)) {
                continue;
            }
            numbers.append(randomNum);
            uniqueSet.insert(randomNum);
        }

        foreach(int i, numbers) {
            faqArray.append(m_aiFAQ[i]);
        }
        QJsonDocument doc(faqArray);
        return QString(doc.toJson(QJsonDocument::Compact));
    }
    case UOS_SYSTEM_ASSISTANT:
    case DEEPIN_SYSTEM_ASSISTANT: {
        //Shuffle the FAQ array
        QRandomGenerator randomGenerator(QDateTime::currentSecsSinceEpoch());
        int arraySize = m_aiFAQ.size();
        arraySize = m_assistantFAQ.size();
        for (int i = arraySize - 1; i > 0; --i) {
            int j = randomGenerator.bounded(i + 1);
            qSwap(m_assistantFAQ[i], m_assistantFAQ[j]);
        }

        int startPos =  m_assistantFAQ.size() > m_limitAQCount ?
                    randomGenerator.bounded(m_assistantFAQ.size() - m_limitAQCount) : 0;

        for (int i = 0; i < m_limitAQCount && i + startPos < m_assistantFAQ.size(); i++) {
            faqArray.append(m_assistantFAQ[i + startPos]);
        }

        QJsonDocument doc(faqArray);
        return QString(doc.toJson(QJsonDocument::Compact));
    }
    case PERSONAL_KNOWLEDGE_ASSISTANT: {
        if (m_persKnowledgeBaseFAQ .isEmpty())
            return "";

        int startPos =  m_persKnowledgeBaseFAQ.size() > m_limitAQCount ?
                    randomGenerator.bounded(m_persKnowledgeBaseFAQ.size() - m_limitAQCount) : 0;

        for (int i = 0; i < m_limitAQCount; i++) {
            faqArray.append(m_persKnowledgeBaseFAQ[i + startPos]);
        }

        QJsonDocument doc(faqArray);
        return QString(doc.toJson(QJsonDocument::Compact));
    }
    case PLUGIN_ASSISTANT: {
        QByteArray faqData = m_aiProxy->getFAQ().toByteArray();
        QJsonArray faqArray = QJsonDocument::fromJson(faqData).array();

        QRandomGenerator randomGenerator(QDateTime::currentSecsSinceEpoch());
        int arraySize = faqArray.size();
        for (int i = arraySize - 1; i >= 0; --i) {
            int j = randomGenerator.bounded(i + 1);
            QJsonValue tmp = faqArray[i];
            faqArray[i] = faqArray[j];
            faqArray[j] = tmp;
        }

        if (faqData.isEmpty())
            return "";
        return QString::fromUtf8(QJsonDocument(faqArray).toJson());
    }
    case AI_WRITING: {
        if (m_aiWritingFAQ.isEmpty())
            return "";
        int startPos =  m_aiWritingFAQ.size() > m_limitAQCount ?
                    randomGenerator.bounded(m_aiWritingFAQ.size() - m_limitAQCount) : 0;

        for (int i = 0; i < m_limitAQCount; i++) {
            faqArray.append(m_aiWritingFAQ[i + startPos]);
        }
        faqArray = m_faqInitTool.assignRandomIcons(faqArray);
        QJsonDocument doc(faqArray);
        return QString(doc.toJson(QJsonDocument::Compact));
    }
    case AI_TEXT_PROCESSING: {
        if (m_aiTextProcessingFAQ.isEmpty())
            return "";
        int startPos =  m_aiTextProcessingFAQ.size() > m_limitAQCount ?
                    randomGenerator.bounded(m_aiTextProcessingFAQ.size() - m_limitAQCount) : 0;

        for (int i = 0; i < m_limitAQCount; i++) {
            faqArray.append(m_aiTextProcessingFAQ[i + startPos]);
        }
        faqArray = m_faqInitTool.assignRandomIcons(faqArray);
        QJsonDocument doc(faqArray);
        return QString(doc.toJson(QJsonDocument::Compact));
    }
    case AI_TRANSLATION: {
        if (m_aiTranslationFAQ.isEmpty())
            return "";
        int startPos =  m_aiTranslationFAQ.size() > m_limitAQCount ?
                    randomGenerator.bounded(m_aiTranslationFAQ.size() - m_limitAQCount) : 0;

        for (int i = 0; i < m_limitAQCount; i++) {
            faqArray.append(m_aiTranslationFAQ[i + startPos]);
        }
        faqArray = m_faqInitTool.assignRandomIcons(faqArray);
        QJsonDocument doc(faqArray);
        return QString(doc.toJson(QJsonDocument::Compact));
    }
    default:
        return "";

    }
}

QString EAiExecutor::getRandomAiFAQByFunction(int type, const QString &function)
{
    QJsonArray faqArray;
    QList<QJsonObject> filteredFaqs;

    // 根据助手类型选择对应的FAQ容器
    QVector<QJsonObject> faqContainer;
    switch (type) {
    case AssistantType::AI_WRITING:
        faqContainer = m_aiWritingFAQ;
        break;
    case AssistantType::AI_TEXT_PROCESSING:
        faqContainer = m_aiTextProcessingFAQ;
        break;
    default:
        return "";
    }

    if (faqContainer.isEmpty()) {
        return "";
    }

    // 过滤出指定Function的FAQ
    for (const auto faq : faqContainer) {
        if (faq["Function"].toString() == function) {
            filteredFaqs.append(faq);
        }
    }

    if (filteredFaqs.isEmpty()) {
        return "";
    }

    // 随机打乱顺序
    QRandomGenerator randomGenerator(QDateTime::currentSecsSinceEpoch());
    int arraySize = filteredFaqs.size();
    for (int i = arraySize - 1; i > 0; --i) {
        int j = randomGenerator.bounded(i + 1);
        qSwap(filteredFaqs[i], filteredFaqs[j]);
    }

    // 取前6个或全部（如果不足6个）
    int count = qMin(6, filteredFaqs.size());
    for (int i = 0; i < count; i++) {
        faqArray.append(filteredFaqs[i]);
    }
    faqArray = m_faqInitTool.assignRandomIcons(faqArray);
    QJsonDocument doc(faqArray);
    return QString(doc.toJson(QJsonDocument::Compact));
}

void EAiExecutor::setChatWindow(ChatWindow *window)
{
    m_chatWindow = window;
    if (currentAssistantId() == "PPT Assistant" || currentAssistantId() == "Poster Assistant") {
        m_chatWindow->setChatButtonVisible(false);
    } else {
        m_chatWindow->setChatButtonVisible(true);
    }
}

void EAiExecutor::showToast(const QString &messge)
{
    if (nullptr != m_chatWindow) {
        m_chatWindow->showToast(messge);
    } else {
        qCWarning(logAIGUI) << "Invalid chatWindow parameter:";
    }
}

void EAiExecutor::getHistoryFromChatInfo()
{
    //初始化 m_convsHistory
    m_convsHistory.clear();
    EaiChatInfoJsonControl::localChatInfoJsonControl().getAllConvsInfo(m_convsHistory);
}

bool EAiExecutor::showChatWindow()
{
    if (nullptr == m_chatWindow) {
        emit ServerWrapper::instance()->sigToLaunchChat(0);
        return true;
    } else {
        m_chatWindow->onlyShowWindow();
        return false;
    }
}

bool EAiExecutor::chatWindowActive() {
    if (m_chatWindow) {
        return m_chatWindow->isActiveWindow();
    }

    return false;
}

void EAiExecutor::closeChatWindow()
{
    if (nullptr != m_chatWindow) {
        m_chatWindow->closeWindow();
    } else {
        qCWarning(logAIGUI) << "Invalid chatWindow parameter:";
    }
}

bool EAiExecutor::startRecorder(int mode)
{
    if (0 == mode) {
        m_chatWindow->setMenuDisabled(true);
        m_chatWindow->setVoiceConversationDisabled(true);
    }
    return AudioControler::instance()->startRecorder();
}

bool EAiExecutor::stopRecorder()
{
    m_chatWindow->setMenuDisabled(false);
    if(!m_isAnswering)
        m_chatWindow->setVoiceConversationDisabled(false);
    return AudioControler::instance()->stopRecorder();
}

bool EAiExecutor::isAudioInputAvailable()
{
    //TODO:
    //    Check input devices

    return AudioControler::audioInputDeviceValid();
}

bool EAiExecutor::isAudioOutputAvailable()
{
    return AudioControler::audioOutputDeviceValid();
}

bool EAiExecutor::playTextAudio(const QString &id, const QString &text, bool isEnd)
{
    return AudioControler::instance()->startAppendPlayText(id, text, isEnd);
}

bool EAiExecutor::stopPlayTextAudio()
{
    return AudioControler::instance()->stopPlayTextAudio();
}

void EAiExecutor::playSystemSound(int effId)
{
    AudioControler::instance()->playSystemSound(
        AudioControler::AudioSystemEffect(effId));
}

QDebug &operator<<(QDebug &debug, const EAiExecutor::AiChatRecord &r)
{
    debug << "ChatRecord {"
          << " question=" << r.question
          << " questHash=" << r.questHash
          << " actType=" << r.actType;

    foreach (auto c, r.replys) {
        debug << "\n"
              << " reqId=" << c.reqId
              << " err=" << c.errCode
              << " errInfo=" << c.errInfo
              << " answer=" << c.answer
              << " llmIcon=" << c.llmIcon
              << " llmName=" << c.llmName
              << "\n";
    }

    debug << "}";
    return debug;
}

bool EAiExecutor::makeJsonFromChatRecord(
    const AiChatRecord &rec,
    QJsonArray &historyArray)
{
    QJsonObject questionObj;
    QJsonObject answerObj;

    questionObj["role"] = "user";
    questionObj["content"] = rec.question;

    QJsonObject docSummaryObj = QJsonDocument::fromJson(rec.douSummaryParam.toUtf8()).object();
    questionObj["fileContent"] = docSummaryObj;

    answerObj["role"] = "assistant";
    QJsonObject obj;
    obj.insert("isShowDoc", false);
    obj.insert("docPath", "");
    obj.insert("iconData", "");
    answerObj["fileContent"] = obj;

    QJsonArray replyArray;

    int errCode = 0;

    foreach (auto r, rec.replys) {
        QJsonObject replyObject;
        if (r.answer.size() > 1) {
            QJsonArray answers;
            foreach (auto a, r.answer) {
                answers.append(a);
            }

            replyObject["content"] = answers;
        } else {
            replyObject["content"] = r.answer.at(0);
        }

        replyObject["talkID"] = r.reqId;
        replyObject["status"] = r.errCode;
        replyObject["type"] = rec.actType;
        replyObject["llmIcon"] = r.llmIcon;
        replyObject["llmName"] = r.llmName;

        if (r.errCode < 0) {
            errCode = r.errCode;
        }

        replyArray.push_back(replyObject);
    }

    answerObj["role"] = "assistant";
    answerObj["anwsers"] = replyArray;

    //Use last error code as record's state
    //TODO:
    //    This Json format is JS defined
    if (errCode < 0) {
        answerObj["status"] = errCode;
    }

    historyArray.push_back(questionObj);
    historyArray.push_back(answerObj);

    return true;
}

QString EAiExecutor::makeAiReqContext(QSharedPointer<EAiPrompt> prompt, bool isFAQGeneration)
{
    QJsonDocument aiCtxDoc;
    QJsonArray ctxArray;
    QString context;
    QString question = prompt->getUserParam();

    if (isFAQGeneration) {
        QStringList docContents = EmbeddingServer::getInstance().getDocContent();
        QString chunk;
        int it = 1;
        for (const QString &docContent : docContents) {
            chunk += QString::number(it) + ":" + docContent + "\n";
            it++;
        }
        QString promptTemplate ="你是问答任务的助手。\n" \
                                "使用以下背景知识生成30个问题%1，输出格式为JSON数组，示例如下：\n" \
                                "```json\n[\n    \"问题1\",\n    \"问题2\"\n]\n```\n" \
                                "背景知识：%2\n" \
                                "问题：";
        if (QLocale::Chinese != m_sysLang.language() || QLocale::SimplifiedChineseScript != m_sysLang.script()) {
            context = promptTemplate.arg("，并翻译成英文，").arg(chunk);
        } else {
            context = promptTemplate.arg("").arg(chunk);
        }
        question = context;
        qCInfo(logAIGUI) << context;
    } else {
        int topK = 5;
        if (m_aiProxy->currentAssistantType() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT) {
            QStringList result = EmbeddingServer::getInstance().searchVecor(question, topK, m_aiProxy->currentAssistantType());
            QString knowleadge;
            for (QString text : result) {
                knowleadge += text;
            }

            if (knowleadge.isEmpty()) {
                QString promptTemplate = "输出答案的开头固定为\"在您的知识库中，未找到相关的信息\"；" \
                                         "然后回答问题：%1\n";
                context = promptTemplate.arg(question);
            } else {
                QString promptTemplate = "你是问答任务的助手。\n" \
                                         "使用以下背景知识来回答问题，不要试图编造答案。如果背景知识中没有问题所需的答案，答案的开头固定为\"在您的知识库中，未找到相关的信息\"；否则答案的开头固定为\"根据您的知识库\"\n" \
                                         "背景知识：%1\n" \
                                         "问题：%2\n" \
                                         "助手答案：";
                context = promptTemplate.arg(knowleadge).arg(question);
            }
            qCInfo(logAIGUI) << context;
        } else if (m_aiProxy->currentAssistantType() == AssistantType::UOS_SYSTEM_ASSISTANT
                   || m_aiProxy->currentAssistantType() == AssistantType::DEEPIN_SYSTEM_ASSISTANT) {
            QStringList result = EmbeddingServer::getInstance().searchVecor(question, topK, m_aiProxy->currentAssistantType());
            QString knowleadge;
            for (QString text : result) {
                knowleadge += text;
            }
            QString promptTemplate = "你是问答任务的助手。\n" \
                                     "使用以下背景知识来回答问题，不要试图编造答案。" \
                                     "如果背景知识中没有问题所需的答案，只输出信息：\"嗯...问到我了，但我会努力去学习这个问题，等我学会了，就能为你解答了。\"" \
                                     "背景知识：%1\n" \
                                     "问题：%2\n" \
                                     "助手答案：";
            context = promptTemplate.arg(knowleadge).arg(question);
            qCInfo(logAIGUI) << context;
        } else if (m_aiProxy->currentAssistantType() == AssistantType::AI_WRITING) {
            QString promptTemplate = "---角色---\n" \
                                     "你是一位专业的写作助手，擅长各种写作风格和格式。\n" \
                                     "---目标---\n" \
                                     "根据用户的需求，创作高质量的内容，满足其特定要求。\n" \
                                     "---写作类型---\n" \
                                     "1. 写文章：撰写结构清晰、论点明确、内容丰富的文章\n" \
                                     "2. 写演讲稿：创作引人入胜的演讲，包含适当的开场、主体和结尾\n" \
                                     "3. 写大纲：为主题创建结构化的提纲\n" \
                                     "4. 写通知：撰写正式的通知或公告\n" \
                                     "5. 写公众号：创作吸引人的社交媒体内容\n" \
                                     "6. 写工作总结：撰写全面的工作报告\n" \
                                     "7. 写调研报告：创建详细的研究报告\n" \
                                     "---输入---\n" \
                                     "%1\n" \
                                     "---输出格式---\n"
                                     "请按以下格式提供内容：\n" \
                                     "标题：\n" \
                                     "内容：";
            context = promptTemplate.arg(question);
            qCInfo(logAIGUI) << context;
        } else if (m_aiProxy->currentAssistantType() == AssistantType::AI_TEXT_PROCESSING) {
            QString promptTemplate = "---角色---\n" \
                                     "你是一位文本处理专家，在语言分析、写作技巧和文本优化方面具有深厚的专业能力。\n" \
                                     "---目标---\n" \
                                     "处理和改进输入文本，同时保持其原意和风格。\n" \
                                     "---处理类型---\n" \
                                     "1. 总结：创建主要观点的简明摘要\n" \
                                     "2. 纠错：识别并修正语法、拼写和文体错误\n" \
                                     "3. 解释：对复杂概念或段落提供详细解释\n" \
                                     "4. 扩写：在保持原有风格的同时，对关键点进行详细阐述\n" \
                                     "5. 续写：以连贯自然的方式继续文本\n" \
                                     "6. 润色：在保持文本本质的同时，提升其清晰度、流畅度和影响力\n" \
                                     "---输入---\n" \
                                     "%1\n" \
                                     "---输出格式---\n"
                                     "请直接输出处理后的文本，不要添加任何额外字符或格式。";
            context = promptTemplate.arg(question);
            qCInfo(logAIGUI)<< context;
        } else if (m_aiProxy->currentAssistantType() == AssistantType::AI_TRANSLATION) {
            QString promptTemplate = "你是一位专业的翻译助手。你的任务是准确自然地翻译给定的文本。\n" \
                                     "翻译输入文本，同时保持其原始含义、语气和风格。如果输入是中文，翻译成英文；如果输入是英文，翻译成中文。\n" \
                                     "%1\n" \
                                     "请直接输出翻译结果，不需要解释。\n" \
                                     "原文：%1\n" \
                                     "翻译：";
            context = promptTemplate.arg(question);
            qCInfo(logAIGUI) << context;
        } else {
            context = prompt->getAiPrompt();
        }
    }

    QString questMd5 = QCryptographicHash::hash(
                           question.toUtf8(),
                           QCryptographicHash::Md5
                       ).toHex();

    //Modify 2020-11-18
    //  refRec.questHash == questMd5 means this
    //retry requset, need skip the record.

    if (m_assistantChatHistories[m_aiProxy->currentAssistantId()].size() > 0) {
        auto &refRec = m_assistantChatHistories[m_aiProxy->currentAssistantId()].last();

        int recCount = m_assistantChatHistories[m_aiProxy->currentAssistantId()].size();

        //Skip the current retury record
        if (refRec.questHash == questMd5) {
            recCount--;
        }

        //Add all histories to context array
        for (int index = 0; index < recCount; index++) {
//            processAiRecord(m_chatHistories.at(index), ctxArray);
            processAiRecord(m_assistantChatHistories[m_aiProxy->currentAssistantId()].at(index), ctxArray);
        }
    }

    //Add Question to context array
    QJsonObject questObj;
    questObj["role"] = "user";
    questObj["content"] = context;
    ctxArray.append(questObj);

    aiCtxDoc.setArray(ctxArray);
    QString ctxJson(aiCtxDoc.toJson(QJsonDocument::Compact));
    qCInfo(logAIGUI) << "AI Request Content: " << ctxJson;
    return ctxJson;
}

QString EAiExecutor::makeContextReq(QSharedPointer<EAiPrompt> prompt, bool isRetry)
{
    if (prompt->singleReqCtx())
        return makeReq(prompt->getAiPrompt());

    QJsonDocument aiCtxDoc;
    QJsonArray ctxArray;

    int convsCount = m_conversionMode == ConversionMode::Normal ? m_currnetConvs.size() : m_currnetPrivateConvs.size();
    if (isRetry)
        convsCount--;
    for (int index = 0; index < convsCount; index++) {
        if (m_conversionMode == ConversionMode::Normal)
            processConversations(m_currnetConvs.at(index), ctxArray);
        else if (m_conversionMode == ConversionMode::Private) {
            processConversations(m_currnetPrivateConvs.at(index), ctxArray);
        }
    }

    //Add Question to context array
    QJsonObject questObj;
    questObj["role"] = "user";
    questObj["content"] = prompt->getAiPrompt();
    ctxArray.append(questObj);

    aiCtxDoc.setArray(ctxArray);
    QString ctxJson(aiCtxDoc.toJson(QJsonDocument::Compact));
    return ctxJson;
}

QString EAiExecutor::makeReq(const QString &prompt)
{
    QJsonDocument aiCtxDoc;
    QJsonArray ctxArray;
    QJsonObject questObj;
    questObj["role"] = "user";
    questObj["content"] = prompt;
    ctxArray.append(questObj);

    aiCtxDoc.setArray(ctxArray);
    QString ctxJson(aiCtxDoc.toJson(QJsonDocument::Compact));
    qCInfo(logAIGUI) << "AI Request Content: " << ctxJson;
    return ctxJson;
}

void EAiExecutor::processAiRecord(
    const AiChatRecord &rec,
    QJsonArray &ctxArray/*out*/)
{
    //{"role": "user", "content": "你好"},
    //{"role": "assistant", "content": "我是人工智能助手"},
    //
    //Modified: 2023-12-12
    //    Now only text chat need to be passed to
    //ai context.Other types are meanless.
    if (ChatAction::ChatTextPlain == rec.actType) {
        if (Q_LIKELY(rec.replys.size() > 0)) {
            QJsonObject questObj;
            QJsonObject answerObj;

            questObj["role"] = "user";
            questObj["content"] = rec.question;

            answerObj["role"] = "assistant";

            QString textAnswer;

            if (Q_LIKELY(rec.replys.size() == 1)) {
                textAnswer = rec.replys.at(0).answer.at(0);

                //Fliter the error conversation
                if (rec.replys.at(0).errCode < 0) {
                    textAnswer = QString("");
                }
            } else {
                //Find the most recently non-empty answer
                //if in retry mode, this is rediculous.:(
                for (auto rIter = rec.replys.rbegin();
                        rIter != rec.replys.rend(); rIter++) {
                    if (!rIter->answer.at(0).isEmpty()
                            && rIter->errCode >= 0) {
                        textAnswer = rIter->answer.at(0);
                        break;
                    }
                }
            }

            answerObj["content"] = textAnswer;

            //Double check the answer isn't empty.
            if (Q_LIKELY(!textAnswer.isEmpty())) {
                ctxArray.append(questObj);
                ctxArray.append(answerObj);
            } else {
                qCWarning(logAIGUI) << "Exception data record:" << rec;
            }
        } else {
            qCWarning(logAIGUI) << "Unexpected data record:" << rec;
        }
    }
}

void EAiExecutor::processConversations(const Conversations &convs, QJsonArray &ctxArray)
{
    if (convs.answers.isEmpty())
        return;

    if (!isAppendConv(convs))
        return;

    QString answerText;
    for (auto rIter = convs.answers.rbegin(); rIter != convs.answers.rend(); rIter++) {
        // 过滤ppt poster
        if (rIter->chatType != ChatAction::ChatTextPlain)
            continue;

        if (!rIter->displayContent.isEmpty() && rIter->errCode >= 0) {
            QJsonDocument doc = QJsonDocument::fromJson(rIter->displayContent.toUtf8());
            if (doc.isArray()) {
                QString contents;
                for (const QJsonValue &value : doc.array()) {
                    QJsonObject obj = value.toObject();
                    if (!obj.contains("chatType"))
                        continue;
                    int chatType = obj["chatType"].toInt();
                    if ((chatType == ChatAction::ChatTextPlain || chatType ==  ChatAction::ChatToolUse) && obj.contains("content")) {
                        contents += obj["content"].toString();
                    }
                }
                if (!contents.isEmpty()) {
                    answerText = contents;
                }
            } else {
                answerText = rIter->displayContent;
            }

            break;
        }
    }

    if (answerText.isEmpty())
        return;

    QJsonObject questObj;
    questObj["role"] = "user";
    questObj["content"] = convs.question.content; // 上下文用prompt，不是用户输入问题

    QJsonObject answerObj;
    answerObj["role"] = "assistant";
    answerObj["content"] = answerText;

    ctxArray.append(questObj);
    ctxArray.append(answerObj);
}

QString EAiExecutor::requestChatText(const QString &llmId, const QString &aiCtx, const QVariantHash &params, QSharedPointer<EAiCallback> callback, EAiPrompt::RequstType type)
{
    QPair<AIServer::ErrorType, QStringList> reply;
    LLMServerProxy tmpLLMAccount = m_aiProxy->queryValidServerAccount(llmId);
    if (!tmpLLMAccount.isValid()) {
        QString uuid = QUuid::createUuid().toString(QUuid::Id128);
        QStringList list;
        list << uuid;
        list << tr("UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
        reply = qMakePair(AIServer::AccountInvalid, list);
    } else if ((m_aiProxy->currentAssistantType() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT
         || m_aiProxy->currentAssistantType() == AssistantType::DEEPIN_SYSTEM_ASSISTANT
         || m_aiProxy->currentAssistantType() == AssistantType::UOS_SYSTEM_ASSISTANT) && (!m_knowledgeBaseExist || !m_embeddingPluginsExist)) {
        QString uuid = QUuid::createUuid().toString(QUuid::Id128);
        QStringList list;
        list << uuid;
        if (!m_knowledgeBaseExist && m_embeddingPluginsExist)
        {
            if(m_aiProxy->currentAssistantType() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT){
                list << tr("The Personal Knowledge Assistant can only be used after configuring the knowledge base.");
                reply = qMakePair(AIServer::PersonalBaseNotExist, list);
            }
            else if(m_aiProxy->currentAssistantType() == AssistantType::DEEPIN_SYSTEM_ASSISTANT
                    || m_aiProxy->currentAssistantType() == AssistantType::UOS_SYSTEM_ASSISTANT)
            {
                if (EAiPrompt::RequstType::General == type)
                    reply = m_aiProxy->requestChatText(llmId, aiCtx, params);
                else if (EAiPrompt::RequstType::Rag == type)
                    reply = m_aiProxy->requestRag(llmId, aiCtx, params);
            }
        }
        else if (!m_embeddingPluginsExist){
            if(m_aiProxy->currentAssistantType() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT)
                list << tr("The Personal Knowledge Assistant can only be used after configuring the model plug.");
            if(m_aiProxy->currentAssistantType() == AssistantType::DEEPIN_SYSTEM_ASSISTANT)
                list << tr("The Deep System Assistant can only be used after configuring the model plug.");
            if(m_aiProxy->currentAssistantType() == AssistantType::UOS_SYSTEM_ASSISTANT)
                list << tr("The UOS System  Assistant can only be used after configuring the model plug.");
            reply = qMakePair(AIServer::PersonalBaseNotExist, list);
        }
    } else {
        if (EAiPrompt::RequstType::General == type)
            reply = m_aiProxy->requestChatText(llmId, aiCtx, params);
        else if (EAiPrompt::RequstType::Rag == type){
            if (!m_knowledgeBaseExist || !m_embeddingPluginsExist) {
                QString uuid = QUuid::createUuid().toString(QUuid::Id128);
                QStringList list;
                list << uuid;
                if (!m_knowledgeBaseExist && m_embeddingPluginsExist)
                {
                    list << tr("Please configure the knowledge base.");
                    reply = qMakePair(AIServer::PersonalBaseNotExist, list);
                }
                else if (!m_embeddingPluginsExist){
                    list << tr("Please install the model plug.");
                    reply = qMakePair(AIServer::PersonalBaseNotExist, list);
                }
            } else {
                reply = m_aiProxy->requestRag(llmId, aiCtx, params);
            }
        }
    }

    QStringList replyList(reply.second);
    QString reqId = replyList.value(0);

    if (reply.first == AIServer::ErrorType::NoError) {
        //<call ID/Socket ID> <signal name>
        //We Only use the call ID
        //Call ID is same as scoket ID
        m_callQueue.insert(reqId, callback);

        if (callback->isStreamMode()) {
            //EAiStreamHandler will release itself after
            //all of data is readed
            EAiStreamHandler *aiStreamHandler = new EAiStreamHandler(
                        reqId, callback);
            aiStreamHandler->process();
        }
        m_isAnswering = true;
    } else {
        //Handle the no account error code
        int errCode = reply.first;
        QString errInfo(reply.second.value(1));

        //the notify should be delay after the request is return.
        QTimer::singleShot(30, this, [callback, errInfo, errCode] {
            QJsonObject message;
            QJsonObject content;
            content.insert("content", errInfo);
            content.insert("chatType", ChatAction::ChatTextPlain);
            message.insert("message", content);
            message.insert("stream", true);
            QString errString = QString(QJsonDocument(message).toJson());
            callback->notify(errString, -errCode);
        });

        qCInfo(logAIGUI) << "reqId:" << reqId
                << " error:" << errCode
                << " errorString:" << errInfo;

        m_isAnswering = false;
    }

    if (m_chatWindow) {
        m_chatWindow->setVoiceConversationDisabled(m_isAnswering);
        m_chatWindow->setHasChatHistory(true);
    }

    return reqId;
}

QString EAiExecutor::requestFunction(const QString &llmId, const QString &aiCtx, const QVariantHash &params, QSharedPointer<EAiCallback> callback, const QJsonArray &funcs)
{
    QPair<AIServer::ErrorType, QStringList> reply;
    LLMServerProxy tmpLLMAccount = m_aiProxy->queryValidServerAccount(llmId);
    if (!tmpLLMAccount.isValid()) {
        QString uuid = QUuid::createUuid().toString(QUuid::Id128);
        QStringList list;
        list << uuid;
        list << tr("UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
        reply = qMakePair(AIServer::AccountInvalid, list);
    } else {
        reply = m_aiProxy->requestFunction(llmId, aiCtx, funcs, params);
    }

    QStringList replyList(reply.second);
    QString reqId = replyList.value(0);

    if (reply.first == AIServer::ErrorType::NoError) {
        //<call ID/Socket ID> <signal name>
        //We Only use the call ID
        //Call ID is same as scoket ID
        m_callQueue.insert(reqId, callback);

        if (callback->isStreamMode()) {
            //EAiStreamHandler will release itself after
            //all of data is readed
            EAiStreamHandler *aiStreamHandler = new EAiStreamHandler(
                        reqId, callback);
            aiStreamHandler->process();
        }
        m_isAnswering = true;
    } else {
        //Handle the no account error code
        int errCode = reply.first;
        QString errInfo(reply.second.value(1));

        //the notify should be delay after the request is return.
        QTimer::singleShot(30, this, [callback, errInfo, errCode] {
            QJsonObject message;
            QJsonObject content;
            content.insert("content", errInfo);
            content.insert("chatType", ChatAction::ChatTextPlain);
            message.insert("message", content);
            message.insert("stream", true);
            QString errString = QString(QJsonDocument(message).toJson());
            callback->notify(errString, -errCode);
        });

        qCInfo(logAIGUI) << "reqId:" << reqId
                << " error:" << errCode
                << " errorString:" << errInfo;

        m_isAnswering = false;
    }

    if (m_chatWindow) {
        m_chatWindow->setVoiceConversationDisabled(m_isAnswering);
        m_chatWindow->setHasChatHistory(true);
    }

    return reqId;
}

QString EAiExecutor::chatRequest(const QString &llmId, const QString &aiCtx, const QVariantHash &params, QSharedPointer<EAiCallback> callback)
{
    QString reqId = m_aiProxy->chatRequest(llmId, aiCtx, params);
    m_callQueue.insert(reqId, callback);

    if (callback->isStreamMode()) {
        EAiStreamHandler *aiStreamHandler = new EAiStreamHandler(reqId, callback);
        aiStreamHandler->process();

        m_isAnswering = true;
    }

    if (m_chatWindow) {
        m_chatWindow->setVoiceConversationDisabled(m_isAnswering);
        m_chatWindow->setHasChatHistory(true);
    }

    return reqId;
}

QString EAiExecutor::requestGenImage(const QString &llmId, const QString &aiCtx, QSharedPointer<EAiCallback> callback)
{
    QString reqId = m_aiProxy->requestGenImage(llmId, aiCtx);
    m_callQueue.insert(reqId, callback);

    m_isAnswering = true;
    if (m_chatWindow) {
        m_chatWindow->setVoiceConversationDisabled(m_isAnswering);
        m_chatWindow->setHasChatHistory(true);
    }

    return reqId;
}

QString EAiExecutor::searchRequest(const QString &llmId, const QString &aiCtx, QSharedPointer<EAiCallback> callback)
{
    QString reqId = m_aiProxy->searchRequest(llmId, aiCtx);
    m_callQueue.insert(reqId, callback);

    m_isAnswering = true;
    if (m_chatWindow) {
        m_chatWindow->setVoiceConversationDisabled(m_isAnswering);
        m_chatWindow->setHasChatHistory(true);
    }

    return reqId;
}

QString EAiExecutor::requestMcpAgent(const QString &llmId, const QString &aiCtx, const QVariantHash &params, QSharedPointer<EAiCallback> callback)
{
    QPair<AIServer::ErrorType, QStringList> reply;
    LLMServerProxy tmpLLMAccount = m_aiProxy->queryValidServerAccount(llmId);
    if (!tmpLLMAccount.isValid()) {
        QString uuid = QUuid::createUuid().toString(QUuid::Id128);
        QStringList list;
        list << uuid;
        list << tr("UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
        reply = qMakePair(AIServer::AccountInvalid, list);
    } else {
        reply = m_aiProxy->requestMcpAgent(llmId, aiCtx, params);
    }

    QStringList replyList(reply.second);
    QString reqId = replyList.value(0);

    if (reply.first == AIServer::ErrorType::NoError) {
        //<call ID/Socket ID> <signal name>
        //We Only use the call ID
        //Call ID is same as scoket ID
        m_callQueue.insert(reqId, callback);

        if (callback->isStreamMode()) {
            //EAiStreamHandler will release itself after
            //all of data is readed
            EAiStreamHandler *aiStreamHandler = new EAiStreamHandler(
                        reqId, callback);
            aiStreamHandler->process();
        }
        m_isAnswering = true;
    } else {
        //Handle the no account error code
        int errCode = reply.first;
        QString errInfo(reply.second.value(1));

        //the notify should be delay after the request is return.
        QTimer::singleShot(30, this, [callback, errInfo, errCode] {
            QJsonObject message;
            QJsonObject content;
            content.insert("content", errInfo);
            content.insert("chatType", ChatAction::ChatTextPlain);
            message.insert("message", content);
            message.insert("stream", true);
            QString errString = QString(QJsonDocument(message).toJson());
            callback->notify(errString, -errCode);
        });

        qCInfo(logAIGUI) << "reqId:" << reqId
                << " error:" << errCode
                << " errorString:" << errInfo;

        m_isAnswering = false;
    }

    if (m_chatWindow) {
        m_chatWindow->setVoiceConversationDisabled(m_isAnswering);
        m_chatWindow->setHasChatHistory(true);
    }

    return reqId;
}

QString EAiExecutor::wordWizardRequest(const QString &llmId, const QString &aiCtx, QSharedPointer<EAiCallback> callback)
{
    if (llmId.isEmpty()) {
        // 随航对应UOSAI助手的模型，若无模型则错误
        QString errInfo = tr("UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
        int errCode = AIServer::AccountInvalid;
        QJsonObject message;
        QJsonObject content;
        content.insert("content", errInfo);
        content.insert("chatType", ChatAction::ChatTextPlain);
        message.insert("message", content);
        message.insert("stream", true);
        errInfo = QString(QJsonDocument(message).toJson());
        callback->notify(errInfo, -errCode);
        return "";
    }

    QVariantHash params = {
        {PREDICT_PARAM_STREAM, QVariant(callback->isStreamMode())},
        {PREDICT_PARAM_THINKCHAIN, QVariant(false)},
        {PREDICT_PARAM_INCREASEUSE, QVariant(true)}
    };
    QString reqId = m_aiProxy->chatRequest(llmId, aiCtx, params);
    m_callQueue.insert(reqId, callback);

    if (callback->isStreamMode()) {
        EAiStreamHandler *aiStreamHandler = new EAiStreamHandler(reqId, callback);
        aiStreamHandler->process();
    }

    return reqId;
}

QSharedPointer<EAiPrompt> EAiExecutor::initPrompt(const QString &userQuestion, const QJsonArray &extention)
{
    QSharedPointer<EAiPrompt> aiPrompt;

    QString functionButton = "";
    int extType = ExtentionType::None;
    QJsonObject extObj;
    for (auto ext : extention) {
        extObj = ext.toObject();
        extType = extObj.value("type").toInt();

        // 一次问答最多选择一个功能
        if (extType == ExtentionType::FunctionButton) {
            functionButton = extObj.value("functionButton").toString() + ":";
            break;
        }
    }

    AssistantType assistantType = m_aiProxy->currentAssistantType();
    switch (assistantType) {
    case AssistantType::UOS_SYSTEM_ASSISTANT:
    case AssistantType::DEEPIN_SYSTEM_ASSISTANT:
    case AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT:
        aiPrompt.reset(new EConversationPrompt(userQuestion));
        aiPrompt->setReqType(EAiPrompt::RequstType::Rag);
        break;
    case AssistantType::AI_WRITING:
        aiPrompt.reset(new EAiWritingPrompt(functionButton + userQuestion));
        aiPrompt->setReqType(EAiPrompt::RequstType::TextPlain);
        break;
    case AssistantType::AI_TEXT_PROCESSING:
        aiPrompt.reset(new EAiTextProcessingPrompt(functionButton + userQuestion));
        aiPrompt->setReqType(EAiPrompt::RequstType::TextPlain);
        break;
    case AssistantType::AI_TRANSLATION:
        aiPrompt.reset(new EAiTranslationPrompt(userQuestion));
        aiPrompt->setReqType(EAiPrompt::RequstType::TextPlain);
        break;
    case AssistantType::UOS_AI:
    default:
        aiPrompt = initDefaultPrompt(userQuestion, extention);
        break;
    }

    // tid:1001600008 event:assistantchat
    ReportIns()->writeEvent(report::AssistantChatPoint(m_aiProxy->currentAssistantDisplayNameEn()).assemblingData());

    return aiPrompt;
}

QSharedPointer<EAiPrompt> EAiExecutor::initDefaultPrompt(const QString &userQuestion, const QJsonArray &extetion)
{
    QSharedPointer<EAiPrompt> aiPrompt;

    if (extetion.isEmpty()) {
        aiPrompt.reset(new EConversationPrompt(userQuestion));
        aiPrompt->setReqType(EAiPrompt::RequstType::General);
        return aiPrompt;
    }

    int extType = ExtentionType::None;
    QJsonObject extObj;
    for (auto ext : extetion) {
        extObj = ext.toObject();
        extType = extObj.value("type").toInt();

        // 目前定义Ext中只有一个可以构造Prompt
        if (extType == ExtentionType::WordSelection || extType == ExtentionType::Instruction || extType == ExtentionType::Conversation)
            break;


        if (extType == ExtentionType::MCPStatus) {
            auto servers = getMcpServers(kDefaultAgentName);
            if (servers.isEmpty())
                extType = ExtentionType::Conversation;
            break;
        }
    }

    QString aiData;
    switch (extType) {
    case ExtentionType::WordSelection:
        // 重试的请求走UOS-AI的接口
        aiData = extObj.value("prompt").toString();
        aiPrompt.reset(new EAiWordSelectionPrompt(aiData));
        aiPrompt->setReqType(EAiPrompt::RequstType::TextPlain);
        break;
    case ExtentionType::Instruction:
        // do inst task
        // TODO: 指令后续需要构造Prompt - promptType aiData
        if (currentAssistantType()==AssistantType::PLUGIN_ASSISTANT) {
            QString userSend = QString("{\"value\": \"%1\", \"tagType\": %2}").arg(userQuestion).arg(extObj.value("tagType").toInt());
            aiPrompt.reset(new EConversationPrompt(userSend));
            aiPrompt->setReqType(EAiPrompt::RequstType::General);
            aiPrompt->setSingleReqCtx(true);
        } else {
            aiPrompt = InstructionManager::instance()->genPrompt(extObj.value("tagType").toInt(), userQuestion);
            aiPrompt->setSingleReqCtx(true);
        }
        break;
    case ExtentionType::MCPStatus:
        aiPrompt.reset(new EConversationPrompt(userQuestion));
        aiPrompt->setReqType(EAiPrompt::RequstType::McpAgent);
        break;
    case ExtentionType::KnowledgeBaseBtnStatus:
        // uos-ai走知识库流程
        aiPrompt.reset(new EConversationPrompt(userQuestion));
        aiPrompt->setReqType(EAiPrompt::RequstType::Rag);
        break;
    case ExtentionType::Conversation:
    default:
        if (currentAssistantType()==AssistantType::PLUGIN_ASSISTANT) {
            std::string stdStrContents = DocParser::convertFile(extObj.value("docPath").toString().toStdString());
            QString contents = Util::textEncodingTransferUTF8(stdStrContents);
            QString isDocEmpty = "false";
            if (contents.trimmed().isEmpty()) {
                isDocEmpty = "true";
            }
            QString userSend = QString("{\"docPath\": \"%1\", \"isDocEmpty\": %2}").arg(extObj.value("docPath").toString()).arg(isDocEmpty);
            aiPrompt.reset(new EConversationPrompt(userSend));
            aiPrompt->setReqType(EAiPrompt::RequstType::General);
        } else {
            aiPrompt.reset(new EConversationPrompt(userQuestion));
            aiPrompt->setReqType(EAiPrompt::RequstType::General);
        }
        break;
    }

    return aiPrompt;
}

QSharedPointer<EAiCallback> EAiExecutor::initCallback(QObject *caller, const QString &notifier, QSharedPointer<EAiPrompt> prompt)
{
    QSharedPointer<EAiCallback> aiCallback;
    aiCallback = initDefaultCallback(caller, notifier);

    if (prompt->instType() == InstructionManager::None)
        return aiCallback;

    // 指令CallBack
    aiCallback.reset(new EAiInstructionCallback(caller));
    aiCallback->setDataMode(EAiCallback::StreamMode);  /* 先固定流模式；需根据aiaction类型选择对于mode */

    if (prompt->instType() == InstructionManager::SearchOnline)
        aiCallback->setDataMode(EAiCallback::CacheMode);

    aiCallback->setNotifier(notifier);
    dynamic_cast<EAiInstructionCallback*>(aiCallback.data())->setInstType(prompt->instType());

    return aiCallback;
}

QSharedPointer<EAiCallback> EAiExecutor::initDefaultCallback(QObject *caller, const QString &notifier)
{
    QSharedPointer<EAiCallback> aiCallback;
    aiCallback.reset(new EAiCallback(caller));
    //aiCallback->setOp(aiAction);       /* 暂时弃用 */
    aiCallback->setDataMode(EAiCallback::StreamMode); /* 先固定流模式；需根据aiaction类型选择对于mode */
    aiCallback->setNotifier(notifier);

    return aiCallback;
}

void EAiExecutor::parseReceivedFaq(const QString &faqContent)
{
    m_isFAQGenerating = false;
    // 个人知识库问题生成结果

    int startIndex = faqContent.indexOf('{');
    int endIndex = faqContent.indexOf('}');
    if (startIndex < 0 || endIndex < startIndex)
        return;

    QJsonObject faqObj;
    QString qaContent = faqContent.mid(startIndex, endIndex - startIndex + 1);
    faqObj = QJsonDocument::fromJson(qaContent.toUtf8()).object();

    QStringList cnFaqList;
    QStringList enFaqList;

    for (auto faq : faqObj.value("chinese").toArray()) {
        cnFaqList.append(faq.toString());
    }
    for (auto faq : faqObj.value("english").toArray()) {
        enFaqList.append(faq.toString());
    }

    if (cnFaqList.size() < 6 || enFaqList.size() < 6)
        return;

    m_persKnowledgeBaseFAQ.clear();
    emit knowledgeBaseFAQGenFinished();

    QString currentLang = "zh_CN";
    if (QLocale::Chinese != m_sysLang.language() || QLocale::SimplifiedChineseScript != m_sysLang.script()) {
        currentLang = "en_US";
    }

    updateFaqList(cnFaqList, "zh_CN", currentLang);
    updateFaqList(enFaqList, "en_US", currentLang);
}

void EAiExecutor::updateFaqList(const QStringList &faqList, const QString &lang, const QString &currentLang)
{
    QJsonArray jsonArr;
    for (int i = 0; i < faqList.size(); i++) {
        QJsonObject faqObject = {
            {"iconName", QString::number(i % 10 + 1) + "-question-personal"},
            {"Question", faqList[i]},
            {"iconPrefix", "icons/"}};
        jsonArr.append(faqObject);

        if (lang == currentLang)
            m_persKnowledgeBaseFAQ.append(faqObject);
    }

    QJsonDocument jsonDoc(jsonArr);
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (dataDir.exists() == false) {
        dataDir.mkpath(dataDir.path());
    }
    QFile file(dataDir.path() + QString("/personal-faq-%1.json").arg(lang));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(jsonDoc.toJson());
        file.close();
    }
}

bool EAiExecutor::isKnowAssistantType()
{
    return m_aiProxy->currentAssistantType() == AssistantType::UOS_SYSTEM_ASSISTANT
            || m_aiProxy->currentAssistantType() == AssistantType::DEEPIN_SYSTEM_ASSISTANT
            || m_aiProxy->currentAssistantType() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT;
}

QString EAiExecutor::appendDocContent(const QJsonArray &exts, const QString &userData)
{
    ConversionType currentConversionType = ConversionType::Text;
    QJsonArray files;
    
    // 首先遍历所有扩展项，检查是否存在MCPStatus
    bool isOpenMcp = false;
    for (auto ext : exts) {
        QJsonObject extObj = ext.toObject();
        if (extObj.value(kExtentionType).toInt() == ExtentionType::MCPStatus) {
            isOpenMcp = true;
            break;
        }
    }
    
    for (auto ext : exts) {
        QJsonObject extObj = ext.toObject();
        if (extObj.value(kExtentionType).toInt() == ExtentionType::DocSummary) {
            files = extObj.value(kExtFile).toArray();
            break;
        }
    }

    if (files.isEmpty()) {
        writeAssistantChatTypeEvent(currentConversionType);
        return userData;
    }

    QString docContent;
    QString imageContent;
    QString docPaths;
    bool hasDocument = false;
    bool hasImage = false;
    
    for (auto file : files) {
        QJsonObject fileDoc = file.toObject();
        if (isOpenMcp){
            QString docPath = fileDoc.value(kExtFileMetaInfo).toObject().value(kExtFileMetaInfoDocPath).toString();
            docPaths += QString("\n文档路径：%1\n").arg(docPath);
            continue;
        }
        if (fileDoc.value(kExtFileType).toInt() == ExtFileType::document) {
            docContent += QString("\n文档内容：%1\n").arg(fileDoc.value(kExtFileContent).toString());
            hasDocument = true;
        } else if (fileDoc.value(kExtFileType).toInt() == ExtFileType::image) {
            imageContent += QString("\n图片内容：%1\n").arg(fileDoc.value(kExtFileContent).toString());
            hasImage = true;
        }
    }

    // 根据检测到的文档和图片类型设置转换类型
    if (hasDocument && hasImage) {
        currentConversionType = ConversionType::TextImageFile;
    } else if (hasDocument) {
        currentConversionType = ConversionType::TextFile;
    } else if (hasImage) {
        currentConversionType = ConversionType::TextImage;
    }

    QString question = userData + docContent + imageContent + docPaths;

    if (question.length() > 2e6) {
        question = question.left(2e6);
    }

    writeAssistantChatTypeEvent(currentConversionType);
    return question;
}

QString EAiExecutor::processRequest(QSharedPointer<EAiPrompt> prompt, QSharedPointer<EAiCallback> callback, const QString &llmId, const ChatChunk &chatChunk)
{
    QString reqId;
    QString aiCtx = makeContextReq(prompt, chatChunk.isRetry);
    LLMServerProxy tmpLLMAccount = m_aiProxy->queryValidServerAccount(llmId); //TODO 数据库操作，请求流程上重复了多次，可优化
    qCInfo(logAIGUI) << "Request Context:" << aiCtx << " Request model:" << tmpLLMAccount.name;

    ReportIns()->writeEvent(report::ModelPoint(tmpLLMAccount).assemblingData());

    QStringList mcpServers;
    QJsonArray extentions = QJsonDocument::fromJson(chatChunk.extention.toUtf8()).array();
    for (auto ext : extentions) {
        QJsonObject extObj = ext.toObject();
        int extType = extObj.value("type").toInt();
        if (extType == ExtentionType::MCPStatus) {
            mcpServers = getMcpServers(kDefaultAgentName);
            break;
        }
    }

    // tid:1001600010 event:private_chat
    if (m_conversionMode == ConversionMode::Private) {
        ReportIns()->writeEvent(report::PrivateChatPoint().assemblingData());
    }

    QVariantHash params = {
        {PREDICT_PARAM_STREAM, QVariant(callback->isStreamMode())},
        {PREDICT_PARAM_THINKCHAIN, QVariant(chatChunk.openThink)},
        {PREDICT_PARAM_ONLINESEARCH, QVariant(chatChunk.onlineSearch)},
        {PREDICT_PARAM_MCPSERVERS, QVariant(mcpServers)}
    };

    // tid:1001600011 event:DigitalChat
    if (m_chatWindow->isDigitalMode()) {
        ReportIns()->writeEvent(report::DigitalChatPoint().assemblingData());
    }

    switch (prompt->reqType()) {
    case EAiPrompt::RequstType::General:
    case EAiPrompt::RequstType::Rag:
        reqId = requestChatText(llmId, aiCtx, params, callback, prompt->reqType());
        break;
    case EAiPrompt::RequstType::TextPlain:
        reqId = chatRequest(llmId, aiCtx, params, callback);
        break;
    case EAiPrompt::RequstType::FunctionCall:
        reqId = requestFunction(llmId, aiCtx, params, callback, prompt->functions());
        break;
    case EAiPrompt::RequstType::Text2Image:
        reqId = requestGenImage(llmId, prompt->getAiPrompt(), callback);
        break;
    case EAiPrompt::RequstType::Search:
        reqId = searchRequest(llmId, aiCtx, callback);
        break;
    case EAiPrompt::RequstType::McpAgent:
        reqId = requestMcpAgent(llmId, aiCtx, params, callback);
        break;
    default:
        break;
    }

    return reqId;
}

bool EAiExecutor::isAppendConv(const Conversations &convs)
{
    QJsonArray exts = QJsonDocument::fromJson(convs.question.extention.toUtf8()).array();
    for (auto ext : exts) {
        QJsonObject extObj = ext.toObject();
        if (extObj.value("type") == ExtentionType::Instruction)
            return false;
    }

    return true;
}

void EAiExecutor::controlAssisConvsLog(const QString &assistantId, const QString &conversationId, const QString &assistantDisplayName, const QVector<uos_ai::Conversations> &convs)
{
    //非隐私会话下，文件保存一份
    if (m_conversionMode == ConversionMode::Normal) {
        //内存保存一份
        m_currnetConvs = convs;
        EaiChatInfoJsonControl::localChatInfoJsonControl().updateConvsInfo(assistantId, conversationId, assistantDisplayName, m_currnetConvs);
    }
    if (m_conversionMode == ConversionMode::Private) {
        //内存保存一份
        m_currnetPrivateConvs = convs;
    }
}

void EAiExecutor::logChatRecord(const QString &reqId,
                                const QString &question,
                                const QStringList &answer,
                                bool isRetry,
                                int err,
                                const QString &errorInfo,
                                int actionType,
                                const QString &llmIcon,
                                const QString &llmName,
                                const QString &docSummaryParam)
{
    AiConversation c;

    c.reqId = reqId;
    c.answer = answer;
    c.errCode = err;
    c.errInfo = errorInfo;
    c.llmIcon = llmIcon;
    c.llmName = llmName;

    //TODO:
    //    Use JS's rule, only -9000, -9001 are flited from
    // conversation context.
    //    This is rediculous.May be need to change.
    //
    // Modified:2023-11-10
    //     Filter error record and empty answer record.
    //BCS these records are meaning less to LLM.Conversations
    //
    // Modified:2023-11-16
    //     Filter only text plain conversation as normal
    //context, other type are filtered
    //
    // Modified:2023-12-12
    //     The context feild is useless,so remove it use
    // actionType instead.

    QString questMd5 = QCryptographicHash::hash(
                           question.toUtf8(),
                           QCryptographicHash::Md5
                       ).toHex();

    if (!isRetry) {
        AiChatRecord rec;

        rec.question = question;
        rec.questHash = questMd5;
        rec.actType = ChatAction(actionType);
        rec.douSummaryParam = docSummaryParam;
        rec.replys.push_back(c);

        //Add to histories set
//        m_chatHistories.push_back(rec);
        m_assistantChatHistories[m_aiProxy->currentAssistantId()].push_back(rec);

    } else {
        //handle retry histories.
        auto &recRef = m_assistantChatHistories[m_aiProxy->currentAssistantId()].last();

        //TODO:
        //    Retry record's type and quest should be same.
        //May be need check md5(recRef.quest)==md5(question),
        //but this check waste time for most case,just assume
        // there are equal.
        if (recRef.questHash == questMd5) {
            if (recRef.actType != ChatAction(actionType)) {
                if (recRef.actType != ChatText2Image) {
                    recRef.actType = ChatAction(actionType);

                    qCInfo(logAIGUI) << "Quest:" << question
                            << " Update type from:" << recRef.actType
                            << " to " << ChatAction(actionType);
                } else {
                    qCWarning(logAIGUI) << "Quest:" << question
                            << " last type is:" << recRef.actType
                            << "do not changed to " << ChatAction(actionType);
                }
            }

            recRef.replys.push_back(c);
        } else {
            qCWarning(logAIGUI) << "Drop retry for different quest:"
                       << " Q1:" << recRef.question
                       << " Q2:" << question;
        }
    }
}

QString EAiExecutor::getChatRecords(bool lastRec)
{
    QJsonDocument historiesDoc;

    QJsonArray historyArray;

    if (m_assistantChatHistories[m_aiProxy->currentAssistantId()].size() > 0) {
        if (lastRec) {

            auto lastRec = m_assistantChatHistories[m_aiProxy->currentAssistantId()].last();

            makeJsonFromChatRecord(lastRec, historyArray);
        } else {

            foreach (auto rec, m_assistantChatHistories[m_aiProxy->currentAssistantId()]) {
                makeJsonFromChatRecord(rec, historyArray);
            }
        }
    }

    historiesDoc.setArray(historyArray);
    QString historiesJson(historiesDoc.toJson(QJsonDocument::Compact));
    return historiesJson;
}

void EAiExecutor::clearChatRecords()
{
    m_assistantChatHistories[m_aiProxy->currentAssistantId()].clear();
    m_chatWindow->setHasChatHistory(false);
}

void EAiExecutor::logConversations(const QString &assistantId, const QString &conversationId, const QString &assistantDisplayName, const QVector<Conversations> &convs)
{
    controlAssisConvsLog(assistantId, conversationId, assistantDisplayName, convs);
}

QString EAiExecutor::getConversations()
{
    QByteArray convsData;

    if (m_conversionMode == ConversionMode::Normal)
        Conversations::convs2Json(convsData, m_currnetConvs);

    if (m_conversionMode == ConversionMode::Private)
        Conversations::convs2Json(convsData, m_currnetPrivateConvs);

    return QString(convsData);
}

QString EAiExecutor::createNewConversation()
{
    m_currnetPrivateConvs.clear();
    m_chatWindow->setHasChatHistory(false);
    return EaiChatInfoJsonControl::localChatInfoJsonControl().createConvs(m_aiProxy->currentAssistantId(), m_conversionMode);
}

void EAiExecutor::removeConversation(const QString &assistantId, const QString &conversationId)
{
    if (EaiChatInfoJsonControl::localChatInfoJsonControl().isConvsExist(assistantId, conversationId))
        EaiChatInfoJsonControl::localChatInfoJsonControl().removeConvs(assistantId, conversationId);
}

void EAiExecutor::removeAllConversation()
{
    EaiChatInfoJsonControl::localChatInfoJsonControl().removeAllConvs();
}

QString EAiExecutor::getConversationHistoryList()
{
    getHistoryFromChatInfo();
    QJsonObject rootObject;
    QMapIterator<QString, QJsonDocument> it(m_convsHistory);
    while (it.hasNext()) {
        it.next();
        rootObject.insert(it.key(), it.value().object());
    }

    QJsonDocument doc(rootObject);

    setTitleBarStatus(true);

    return doc.toJson(QJsonDocument::Indented);  // 使用格式化输出
}

QString EAiExecutor::saveImageAs(const QString &imageData, bool saveAs)
{
    QString imagePath;
    if (saveAs) {
        QString picturesPath = QStandardPaths::writableLocation(
                                   QStandardPaths::PicturesLocation);
        QString imageName = QString("/UosAiTpp-%1.jpg")
                            .arg(QDateTime::currentMSecsSinceEpoch());
        picturesPath += imageName;

        QStringList supportedFormats;
        supportedFormats << "Jpg(*.jpg)"
                         << "Jpeg(*.jpeg)"
                         << "Bitmap(*.bmp)"
                         << "Png(*.png)";

        QStringList formatSuffix;
        formatSuffix << "JPG"
                     << "JPG"
                     << "BMP"
                     << "PNG";

        QString filter = supportedFormats.join(";;");
        QString selectFilter;

        imagePath = DFileDialog::getSaveFileName(
                                m_chatWindow,
                                tr("Export Image As"),
                                picturesPath,
                                filter,
                                &selectFilter);
    } else {
        QString imageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QString("/TEMP");
        if (!QDir(imageDir).exists()) {
            QDir().mkpath(imageDir);
        }
        imagePath = imageDir + QString("/UosAiImage-%1.jpg").arg(QDateTime::currentMSecsSinceEpoch());
    }

    qCInfo(logAIGUI) << "Saving image to:" << imagePath;
    QFile file(imagePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QByteArray::fromBase64(imageData.toUtf8()));
        file.close();
    }

    return imagePath;
}

bool EAiExecutor::previewImage(const QString &imageData)
{
    QString filePath = Util::imageData2TmpFile(m_ttpDir.path(), imageData);
    return previewImageForPath(filePath);
}

bool EAiExecutor::previewImageForPath(const QString &imagePath)
{
    qCInfo(logAIGUI) << "Previewing image:" << imagePath;

    QFileInfo imageInfo(imagePath);

    bool openOK = false;

    if (imageInfo.exists()) {
        openOK = QDesktopServices::openUrl(QUrl::fromLocalFile(imagePath));
    } else {
        qCWarning(logAIGUI) << "Opening URL in default application:" << imagePath;
    }

    return openOK;
}

void EAiExecutor::copyImg2Clipboard(const QString &imageData)
{
    QString filePath = Util::imageData2TmpFile(m_ttpDir.path(), imageData);
    qCInfo(logAIGUI) << "Copying image to clipboard:" << filePath;

    QFileInfo imageInfo(filePath);

    if (imageInfo.exists()) {
        QImageReader imageReader(imageInfo.filePath());
        QImage inputImage = imageReader.read();

        if (!inputImage.isNull()) {
            // 获取剪贴板实例
            QClipboard *clipboard = QGuiApplication::clipboard();

            // 将图片复制到剪贴板
            clipboard->setImage(inputImage);
        } else {
            qCWarning(logAIGUI) << "ImageReader failed:"
                       << " path=" << imageInfo.filePath()
                       << " error=" << imageReader.errorString();
        }
    } else {
        qCWarning(logAIGUI) << "Image file not found:" << filePath;
    }
}

bool EAiExecutor::isNetworkAvailable()
{
    return NetworkMonitor::getInstance().isOnline();
}

void EAiExecutor::personalFAQGenerate()
{
#ifdef ENABLE_ASSISTANT
    if (m_isFAQGenerating && !m_faqId.isEmpty()) {
        m_aiProxy->cancelRequestTask(m_faqId);
    }

    if (!m_knowledgeBaseExist)
        return;

    m_isFAQGenerating = true;

    QString assistantId = m_aiProxy->queryAssistantIdByType(AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT);
    QString llmId = DbWrapper::localDbWrapper().queryLlmIdByAssistantId(assistantId);

    QSharedPointer<EAiCacheCallback> aiCallback;
    aiCallback.reset(new EAiCacheCallback(this));
    aiCallback->setNotifier(QString(""));

    QPair<AIServer::ErrorType, QStringList> reply;
    QSharedPointer<EAiPrompt> aiPrompt(new EAiGenerateQuestionPrompt());
    QVariantHash params = {
        {PREDICT_PARAM_STREAM, QVariant(true)},
        {PREDICT_PARAM_INCREASEUSE, QVariant(false)}
    };
    m_faqId = m_aiProxy->chatRequest(llmId, makeReq(aiPrompt->getAiPrompt()), params);

    EAiStreamHandler *aiStreamHandler = new EAiStreamHandler(m_faqId, aiCallback);
    aiStreamHandler->process();
#endif
}

void EAiExecutor::openInstallWidget(const QString &appname)
{
    LocalModelServer::getInstance().openInstallWidgetOnTimer(appname);
}
void EAiExecutor::onCommandsReceived(const QVariantMap &commands)
{
}

void EAiExecutor::onChatTextReceived(const QString &callId, const QString &chatText)
{
#ifdef AI_DEBUG
    qCInfo(logAIGUI) << "callId=" << callId << " chatText=" << chatText;
#endif

    if (m_faqId == callId) {
        parseReceivedFaq(chatText);
        return;
    }

    auto reqIter = m_callQueue.find(callId);

    if (reqIter != m_callQueue.end()) {
        //Notify the calling owner the result.
        //We only send the full cached result in
        //cache mode
        if (reqIter->get()->isStreamMode()) {
            QJsonObject message;
            QJsonObject content;
            content.insert("content", QString(""));
            content.insert("chatType", ChatAction::ChatTextPlain);
            message.insert("message", content);
            message.insert("stream", true);
            QString errInfo = QString(QJsonDocument(message).toJson());
            reqIter->get()->notify(errInfo, EAiStreamHandler::StreamEnd);
        } else {
            reqIter->get()->notify(chatText, 0);
        }

        m_callQueue.erase(reqIter);
    }
    m_isAnswering = false;
    if (m_chatWindow) {
        m_chatWindow->setVoiceConversationDisabled(false);
    }
}

void EAiExecutor::onChatError(const QString &id, int code, const QString &errorString)
{
    qCWarning(logAIGUI) << "Chat error occurred:"
                        << "id=" << id 
                        << "code=" << code
                        << "error=" << errorString;

    if (code == AIServer::NetworkError) {
        qCWarning(logAIGUI) << "Network error detected";
        emit netStateChanged(false);
    }
    
    //Reverse the error code if positive
    if (code > 0) {
        code = -code;
    }

    auto reqIter = m_callQueue.find(id);

    if (reqIter != m_callQueue.end()) {
        QJsonObject message;
        QJsonObject content;
        content.insert("content", errorString);
        content.insert("chatType", ChatAction::ChatTextPlain);
        message.insert("message", content);
        message.insert("stream", false);
        QString errJson = QString(QJsonDocument(message).toJson());

        reqIter->get()->notify(errJson, code);

        m_callQueue.erase(reqIter);
    }

    m_isAnswering = false;
    if (m_chatWindow) {
        m_chatWindow->setVoiceConversationDisabled(false);
    }
}

void EAiExecutor::onTextToPictureData(const QString &id, const QList<QByteArray> &imageData)
{
    QStringList imagePaths;

    foreach (auto p, imageData) {
        imagePaths << p;
    }

    emit textToPictureFinish(id, imagePaths);
    m_isAnswering = false;
    m_chatWindow->setVoiceConversationDisabled(false);
}

void EAiExecutor::onPPTCreated(const QString &id, const QList<QByteArray> &imageData)
{
    QStringList imagePaths;

    foreach (auto p, imageData) {
        imagePaths << p;
    }

    if (currentAssistantId() == "PPT Assistant") {
        emit pptCreateSuccess(id, imagePaths);
    } else {
        QJsonArray contentJsonArray;
        for (const QString &str : imagePaths) {
            contentJsonArray.append(str);
        }
        QJsonDocument contentJsonDoc(contentJsonArray);
        QString contentJsonString = contentJsonDoc.toJson(QJsonDocument::Compact);

        createPPTHistory(id, contentJsonString);
    }

    m_isAnswering = false;
    m_chatWindow->setVoiceConversationDisabled(false);
}

void EAiExecutor::createPPTHistory(const QString &id, const QString &displayContent)
{
    Conversations conv;
    // question
    conv.question.chatType       = ChatAction::ChatTextPlain;
    conv.question.content        = "";
    conv.question.displayContent = "";
    conv.question.displayHash    = "";
    conv.question.llmIcon        = "";
    conv.question.llmName        = "";
    conv.question.llmId          = "";
    conv.question.llmModel       = 0;
    conv.question.errCode        = 0;
    conv.question.errInfo        = "";
    conv.question.isRetry        = false;
    conv.question.extention      = "";
    conv.question.reqId          = "";

    // answer
    ChatChunk answer;
    answer.assistantId    = "PPT Assistant";
    answer.chatType       = ChatAction::ChatText2Image;
    answer.reqId          = "0";
    answer.content        = "";
    answer.displayContent = displayContent;
    answer.displayHash    = "";
    answer.llmIcon        = m_aiProxy->queryIconById("PPT Assistant", "AI PPT");
    answer.llmName        = "AI PPT";
    answer.llmId          = "AI PPT";
    answer.llmModel       = 3000;
    answer.errCode        = 0;
    answer.errInfo        = "";
    answer.isRetry        = false;
    answer.extention      = QString("[{\"type\":6,\"idValue\":%1}]").arg(id);
    answer.knowledgeSearchStatus = false;
    conv.answers.append(answer);

    // 获取PPT助手的最新会话
    QString json = EaiChatInfoJsonControl::localChatInfoJsonControl().findLatestConversation("PPT Assistant");
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject obj = doc.object();
    QString conversationId = obj["conversationId"].toString();

    // 更新对话记录
    QVector<uos_ai::Conversations> convs;
    EaiChatInfoJsonControl::localChatInfoJsonControl().getConvsInfo("PPT Assistant", conversationId, convs);
    convs.push_back(conv);
    if (!conversationId.isEmpty()) {
        EaiChatInfoJsonControl::localChatInfoJsonControl().updateConvsInfo(
            "PPT Assistant",
            conversationId,
            m_aiProxy->queryDisplayNameById("PPT Assistant"),
            convs
        );
    } else {
        qCWarning(logAIGUI) << "No valid PPT assistant conversation record found";
    }
}

void EAiExecutor::onPPTChanged(const QString &id, const QList<QByteArray> &imageData)
{
    QStringList imagePaths;

    foreach (auto p, imageData) {
        imagePaths << p;
    }

    emit pptChangeSuccess(id, imagePaths);
    m_isAnswering = false;
    m_chatWindow->setVoiceConversationDisabled(false);
}

void EAiExecutor::onPosterCreated(const QList<QString> &idList, const QList<QByteArray> &imageData)
{
    QStringList imagePaths;

    foreach (auto p, imageData) {
        imagePaths << p;
    }
    if (currentAssistantId() == "Poster Assistant") {
        emit posterCreateSuccess(idList, imagePaths);
    } else {
        QJsonArray contentJsonArray;
        for (const QString &str : imagePaths) {
            contentJsonArray.append(str);
        }
        QJsonDocument contentJsonDoc(contentJsonArray);
        QString contentJsonString = contentJsonDoc.toJson(QJsonDocument::Compact);

        QJsonArray idJsonArray;
        for (const QString &str : idList) {
            idJsonArray.append(str);
        }
        QJsonDocument idJsonDoc(idJsonArray);
        QString idJsonString = idJsonDoc.toJson(QJsonDocument::Compact);

        createPosterHistory(idJsonString, contentJsonString);
    }
    m_isAnswering = false;
    m_chatWindow->setVoiceConversationDisabled(false);
}

void EAiExecutor::createPosterHistory(const QString &idList, const QString &displayContent)
{
    Conversations conv;
    // question
    conv.question.chatType       = ChatAction::ChatTextPlain;
    conv.question.content        = "";
    conv.question.displayContent = "";
    conv.question.displayHash    = "";
    conv.question.llmIcon        = "";
    conv.question.llmName        = "";
    conv.question.llmId          = "";
    conv.question.llmModel       = 0;
    conv.question.errCode        = 0;
    conv.question.errInfo        = "";
    conv.question.isRetry        = false;
    conv.question.extention      = "";
    conv.question.reqId          = "";

    // answer
    ChatChunk answer;
    answer.assistantId    = "Poster Assistant";
    answer.chatType       = ChatAction::ChatText2Image;
    answer.reqId          = "0";
    answer.content        = "";
    answer.displayContent = displayContent;
    answer.displayHash    = "";
    answer.llmIcon        = m_aiProxy->queryIconById("Poster Assistant", "亦心 AI");
    answer.llmName        = "亦心 AI";
    answer.llmId          = "亦心 AI";
    answer.llmModel       = 3000;
    answer.errCode        = 0;
    answer.errInfo        = "";
    answer.isRetry        = false;
    answer.extention      = QString("[{\"type\":6,\"idValue\":%1}]").arg(idList);
    answer.knowledgeSearchStatus = false;
    conv.answers.append(answer);

    // 获取Poster助手的最新会话
    QString json = EaiChatInfoJsonControl::localChatInfoJsonControl().findLatestConversation("Poster Assistant");
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject obj = doc.object();
    QString conversationId = obj["conversationId"].toString();

    // 更新对话记录
    QVector<uos_ai::Conversations> convs;
    EaiChatInfoJsonControl::localChatInfoJsonControl().getConvsInfo("Poster Assistant", conversationId, convs);
    convs.push_back(conv);
    
    if (!conversationId.isEmpty()) {
        EaiChatInfoJsonControl::localChatInfoJsonControl().updateConvsInfo(
            "Poster Assistant",
            conversationId,
            m_aiProxy->queryDisplayNameById("Poster Assistant"),
            convs
        );
    } else {
        qCWarning(logAIGUI) << "No valid Poster assistant conversation record found";
    }
}

void EAiExecutor::writeAssistantChatTypeEvent(EAiExecutor::ConversionType type)
{
    // tid:1001600013 event:assistant_chat_type
    switch (type) {
    case ConversionType::Text:
        ReportIns()->writeEvent(report::AssistantChatTypePoint("Text").assemblingData());
        break;
    case ConversionType::TextFile:
        ReportIns()->writeEvent(report::AssistantChatTypePoint("TextFile").assemblingData());
        break;
    case ConversionType::TextImage:
        ReportIns()->writeEvent(report::AssistantChatTypePoint("TextImage").assemblingData());
        break;
    case ConversionType::TextImageFile:
        ReportIns()->writeEvent(report::AssistantChatTypePoint("TextImageFile").assemblingData());
        break;
    default:
        return;
    }
}

void EAiExecutor::JudgeIsShowFreeAccountGuide()
{
    auto* watcher = new QFutureWatcher<QNetworkReply::NetworkError>(this);

    QFuture<QNetworkReply::NetworkError> future = QtConcurrent::run([=]() {
        auto displayError = UosFreeAccounts::instance().freeAccountButtonDisplay("account", m_hasActivity);
        int status = 0;
        auto freeModelError = UosFreeAccounts::instance().checkFreeModelActivity(DeepSeek_Uos_Free, status, m_hasModelActivity);

        if (displayError != QNetworkReply::NoError)
            return displayError;
        if (freeModelError != QNetworkReply::NoError)
            return freeModelError;

        return QNetworkReply::NoError;
    });

    connect(watcher, &QFutureWatcher<QNetworkReply::NetworkError>::finished, this, [ = ]() {
        QNetworkReply::NetworkError result = watcher->future().result();
        if (result == QNetworkReply::NoError && m_hasActivity.display == 1 && m_hasModelActivity.inActivityPeriod) {
            m_isShwoFreeAccountGuide = true;
        } else {
            m_isShwoFreeAccountGuide = false;
        }
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

bool EAiExecutor::showLostFileWarningDlg(const QString lostFileList)
{
    return m_chatWindow->showLostFileWarningDlg(lostFileList);
}

bool EAiExecutor::showInstallUosAIAgentDlg()
{
    return m_chatWindow->showInstallUosAIAgentDlg(UOSAIAGENTNAME);
}

bool EAiExecutor::getFreeCredits(bool isShowDlg)
{
    if ((isShowDlg && m_chatWindow->showGetFreeCreditsDlg()) || !isShowDlg) {
        // 领取免费额度
        QString listJson = m_aiProxy->queryLLMAccountList();

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(listJson.toUtf8(), &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(logAIGUI) << "Failed to parse LLM account list JSON:" << parseError.errorString();
            return false;
        }
        
        if (!jsonDoc.isArray()) {
            qCWarning(logAIGUI) << "LLM account list is not a JSON array";
            return false;
        }
        
        QJsonArray accountArray = jsonDoc.array();
        QString modelIdForFreeAccount;
        
        for (const QJsonValue &accountValue : accountArray) {
            if (!accountValue.isObject()) {
                continue;
            }
            
            QJsonObject accountObj = accountValue.toObject();
            if (accountObj.contains("model") && accountObj["model"].isDouble()) {
                int modelType = accountObj["model"].toInt();
                if (modelType == DeepSeek_Uos_Free && accountObj.contains("id") && accountObj["id"].isString()) {
                    modelIdForFreeAccount = accountObj["id"].toString();
                    qCInfo(logAIGUI) << "Found model ID for type 81:" << modelIdForFreeAccount;
                    break;
                }
            }
        }
        
        if (modelIdForFreeAccount.isEmpty()) {
            qCWarning(logAIGUI) << "No model found with type DeepSeek_Uos_Free";
            return false;
        }
        
        m_aiProxy->claimUsageRequest(modelIdForFreeAccount);
        return true;
    }

    return false;
}

QString EAiExecutor::getCurrentShortcut()
{
    return m_chatWindow->getCurrentShortcut();
}

bool EAiExecutor::isSimplifiedChinese()
{
    const QString locale = QLocale::system().name().simplified();
    const QString shortLocale = Util::splitLocaleName(locale);
    if (shortLocale == "zh"){
        return true;
    }else {
        return false;
    }

}

void EAiExecutor::onAddToServerStatusChanged(const QStringList &files, int status)
{
    Q_UNUSED(files)

    if (status != 1)
        return;

    m_knowledgeBaseExist = true;
    qCInfo(logAIGUI) << "Knowledge base status changed, files:" << files;
    emit knowledgeBaseExistChanged(m_knowledgeBaseExist);

    personalFAQGenerate();
}

void EAiExecutor::onIndexDeleted(const QStringList &files)
{
    //qDebug() << "IndexDeleted, " << files;
    if (files.isEmpty())
        return;

    bool knowBaseExits = EmbeddingServer::getInstance().getDocFiles().size();
    if (!knowBaseExits) {
        m_knowledgeBaseExist = false;
        m_persKnowledgeBaseFAQ.clear();
        m_faqId = "";

        emit knowledgeBaseExistChanged(m_knowledgeBaseExist);

        QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

        for (const QString &lang : QStringList{"zh_CN", "en_US"}) {
            QFile file(dataDir.path() + QString("/personal-faq-%1.json").arg(lang));
            if (file.exists()) {
                // 删除文件
                bool success = file.remove();
                if (success) {
                    qCInfo(logAIGUI) << "personal faq file deleted successfully.";
                } else {
                    qCWarning(logAIGUI) << "Failed to delete personal faq file.";
                }
            }
        }
        return;
    }

    personalFAQGenerate();
}

void EAiExecutor::onLocalLLMStatusChanged(bool isExist)
{
    qCInfo(logAIGUI) << "Local LLM status changed:"
                     << "status=" << (isExist ? "available" : "unavailable");
    emit localLLMStatusChanged(isExist);
}

void EAiExecutor::onEmbeddingPluginsStatusChanged(bool isExist)
{
    qCInfo(logAIGUI) << "Embedding plugins status changed:"
                     << "status=" << (isExist ? "available" : "unavailable");
    m_embeddingPluginsExist = isExist;
    emit embeddingPluginsStatusChanged(isExist);
}

void EAiExecutor::onScreenshotCustomDone(const QString &imagePath)
{
    qCInfo(logAIGUI) << "Screenshot ImagePath:"<<imagePath;
    // 恢复聊天窗口显示
    if(m_chatWindow != nullptr && !m_chatWindow->isActiveWindow()) {
        showChatWindow();
    }

    if (!imagePath.isEmpty()) {
        m_chatWindow->appendImage(imagePath);
    }
    // 断开对 CustomDone 信号的监听
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.disconnect("com.deepin.Screenshot", "/com/deepin/Screenshot", "com.deepin.Screenshot", "CustomDone", this, SLOT(onScreenshotCustomDone(QString)));
}

void EAiExecutor::onScreenshotCallFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<void> reply = *watcher;
    
    if (reply.isError()) {
        qCWarning(logAIGUI) << "Screenshot async call failed:" << reply.error().message();
    } else {
        qCInfo(logAIGUI) << "Screenshot async call completed successfully";
    }
    
    // 清理 watcher
    watcher->deleteLater();
}

void EAiExecutor::onAccountUsageClaimed(bool ret, const QString &msg)
{
    if (m_chatWindow->isDigitalMode()) {
        // show dialog
        m_chatWindow->showGetFreeCreditsResultDlg(ret);
    }

    Q_EMIT sigClaimUsageResult(ret, msg);
}

void EAiExecutor::documentSummarySelect()
{
    EParserDocument::instance()->selectDocument(m_aiProxy->currentAssistantType());
}

QString EAiExecutor::processClipboardData()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *data = clipboard->mimeData();

    if (nullptr == data)
        return QString("");

    QStringList needParserFiles;
    if (data->hasImage()) {
        QVariant imageData = data->imageData();
        if (!imageData.isNull()) {
            QImage paseteImage = imageData.value<QImage>();
            QString imageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QString("/TEMP");
            if (!QDir(imageDir).exists()) {
                QDir().mkpath(imageDir);
            }
            QString imagePath = imageDir + QString("/UosAiImage-%1.jpg").arg(QDateTime::currentMSecsSinceEpoch());
            paseteImage.save(imagePath);
            needParserFiles.append(imagePath);
        }
    } else if (data->hasUrls()) {
        QList<QUrl> urls = data->urls();
        for (auto url : urls) {
            needParserFiles.append(url.path());
        }
    }

    if (!needParserFiles.isEmpty()) {
        EParserDocument::instance()->handleCopyFile(needParserFiles, m_aiProxy->currentAssistantType());
        return QString("");
    }

    return data->text();
}

void EAiExecutor::onDocSummaryDragInView(const QStringList &docPaths, const QString &defaultPrompt)
{
    EParserDocument::instance()->dragInViewDocument(docPaths, defaultPrompt, m_aiProxy->currentAssistantType());
}

void EAiExecutor::documentSummaryParsing(const QString &id, const QString &docPath)
{
    EParserDocument::instance()->parser(id, docPath);
}

void EAiExecutor::openFile(const QString &filePath)
{
    emit openFileFromPathResult(Util::openFileFromPath(filePath));
}

void EAiExecutor::openUrl(const QString &url)
{
    if (Util::launchUosBrowser(url))
        return;

    qCInfo(logAIGUI) << "Opening URL in default browser:" << url;
    Util::launchDefaultBrowser(url);
}

void EAiExecutor::wordWizardContinueChat(const Conversations &conv, int type)
{
    m_wordWizardConvs.enqueue(conv);

    emit sigAppendWordWizardConv(type);
}

void EAiExecutor::wordWizardAskAI(const QString &question, int type)
{
    m_wordWizardQuestions.enqueue(question);

    m_chatWindow->appendAskQuestion(type);
}

void EAiExecutor::appendWordWizardConv(int type)
{
    AssistantType assistantType = static_cast<AssistantType>(type);
    QString uosaiAssistantId = m_aiProxy->queryAssistantIdByType(assistantType);
    QString uosaiDisplayName = m_aiProxy->queryDisplayNameByType(assistantType);

    if (m_wordWizardConvs.isEmpty())
        return;

    QString json = EaiChatInfoJsonControl::localChatInfoJsonControl().findLatestConversation(uosaiAssistantId);
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject obj = doc.object();
    QString uosaiConvsId = obj["conversationId"].toString();

    QVector<uos_ai::Conversations> uosaiConvs;
    EaiChatInfoJsonControl::localChatInfoJsonControl().getConvsInfo(uosaiAssistantId, uosaiConvsId, uosaiConvs);
    uosaiConvs.append(m_wordWizardConvs.head());
    m_wordWizardConvs.dequeue();

    if (m_chatWindow->isDigitalMode()) {
        m_chatWindow->digital2ChatStatusChange();
    }

    setCurrentAssistantId(uosaiAssistantId);

    //存日志
    controlAssisConvsLog(uosaiAssistantId, uosaiConvsId, uosaiDisplayName, uosaiConvs);
}

void EAiExecutor::appendWordWizardQuestion(int type)
{
    AssistantType assistantType = static_cast<AssistantType>(type);
    QString uosaiAssistantId = m_aiProxy->queryAssistantIdByType(assistantType);
    if (m_wordWizardQuestions.isEmpty())
        return;

    QString uosaiQuestions;
    uosaiQuestions = m_wordWizardQuestions.head();
    m_wordWizardQuestions.dequeue();

    if (m_chatWindow->isDigitalMode()) {
        m_chatWindow->digital2ChatStatusChange();
    }

    emit sigWordWizardAsk(uosaiQuestions);
}

void EAiExecutor::webViewLoadFinished()
{
    emit sigWebViewLoadFinished();
}

void EAiExecutor::previewRefDoc(const QString &docPath, const QStringList &docContents)
{
    m_chatWindow->previewReferenceDoc(docPath, docContents);
}

QString EAiExecutor::getInstList()
{
    if (m_aiProxy->currentAssistantType() == AssistantType::PLUGIN_ASSISTANT) {
        return m_aiProxy->currentAssistantInstList();
    }
    return InstructionManager::instance()->query(m_aiProxy->currentAssistantType(), m_aiProxy->currentLLMModel());
}

void EAiExecutor::rateAnwser(const int questionIdx, const int answerIdx, int rate, const QString &extJson)
{
    if (questionIdx < 0 || answerIdx < 0 || rate < 0)
        return;

//    QVector<AiChatRecord> &logs = m_assistantChatHistories[m_aiProxy->currentAssistantId()];
//    if (questionIdx >= logs.size())
//        return;

//    AiChatRecord &cr = logs[questionIdx];
//    if (answerIdx >= cr.replys.size())
//        return;

//    AiConversation &conv = cr.replys[answerIdx];
//    conv.extJson = extJson;
    {
        QVariantHash logData = QJsonDocument::fromJson(extJson.toUtf8()).object().toVariantHash();
        UosRateLog log;
        log.type = UosLogType::AnwserRate;
        log.app = m_aiProxy->appId();
        log.question = logData.value("question").toString();
        log.questionTime = logData.value("questionTime").toString();
        log.answer = logData.value("answer").toString();
        log.answerTime = logData.value("answerTime").toString();
        log.assistantName = logData.value("assistantName").toString();
        log.modelType = logData.value("modelType").toString();
        log.llm = logData.value("llm").toString();
        log.likeOrNot = static_cast<UosRateLog::Rate>(logData.value("likeOrNot").toInt());
        UosSimpleLog::instance().addRateLog(log);
    }
}

void EAiExecutor::setTitleBarStatus(bool status)
{
    m_chatWindow->setTitleBarStatus(status);
}

bool EAiExecutor::showWarningDialog(const QString assistantId, const QString conversationId, const QString msg, bool isDelete, bool isLlmDelete, bool isAllConvDelete)
{
    return m_chatWindow->showWarningDialog(assistantId, conversationId, msg, isDelete, isLlmDelete, isAllConvDelete);
}

bool EAiExecutor::showRmMcpServerDlg(const QString &name)
{
    return m_chatWindow->showRmMcpServerDlg(name);
}

void EAiExecutor::showUpdateDialog(const QString &msg, const QString &appName)
{
    return m_chatWindow->showUpdateDialog(msg, appName);
}

void EAiExecutor::showPromptWindow()
{
    qCInfo(logAIGUI) << "Show prompt window, PRIVACY_UPDATE";
    int width = m_chatWindow->getTitleBarBtnWidth();
    emit sigToShowPromptWindow(width);
}

void EAiExecutor::changeFreeAccountGuide(bool isShowFreeAccountGuide, bool isPreShow)
{
    emit sigToChangeFreeAccountGuide(isShowFreeAccountGuide, isPreShow);
}

void EAiExecutor::updateUpdatePromptDB(bool isClicked)
{
    qCInfo(logAIGUI) << "Update prompt window, PRIVACY_UPDATE:" << isClicked;
    // 使用枚举更新智能体控制位
    DbWrapper::localDbWrapper().updateUpdatePromptBits(UpdatePromptBitType::AUTO_MCP, isClicked ? 1 : 0);
}

void EAiExecutor::updateUpdateFreeAccountGuideDB(bool isClicked)
{
    qCInfo(logAIGUI) << "Update prompt window, PRIVACY_UPDATE:" << isClicked;
    // 使用枚举更新智能体控制位
    DbWrapper::localDbWrapper().updateUpdatePromptBits(UpdatePromptBitType::FREE_CREDITS, isClicked ? 1 : 0);
}

QStringList EAiExecutor::getMcpServers(const QString &agentName)
{
    QSharedPointer<MCPServer> mcpServer = AgentFactory::instance()->getMCPServer(agentName);
    QStringList servers {};
    if (!mcpServer)
        return servers;

    mcpServer->scanServers();
    auto sers = mcpServer->serverNames();
    QStringList enabledMcpList = DConfigManager::instance()->value(MCP_GROUP, MCP_ENABLED_LIST).toStringList();

    for (auto it = sers.begin(); it != sers.end(); ++it) {
        if (enabledMcpList.contains(*it))
            servers.append(*it);
    }

    return servers;
}

bool EAiExecutor::getThirdPartyMcpAgreement()
{
    return m_chatWindow->getThirdPartyMcpAgreement();
}

bool EAiExecutor::isInstallUosAiAgent(const QString &agentName)
{
    QSharedPointer<MCPServer> mcpServer = AgentFactory::instance()->getMCPServer(agentName);
    if (mcpServer){
        return mcpServer->isRuntimeReady();
    }
    return false;
}

void EAiExecutor::startScreenshot()
{
    if (m_isAnswering || currentAssistantType() == AssistantType::PLUGIN_ASSISTANT) {
        return;
    }
    if (m_inputFileSize >= 3) {
        if(m_chatWindow != nullptr && !m_chatWindow->isActiveWindow()) {
            showChatWindow();
        }
        sigShowTip(tr("You can upload up to 3 files or image"));
        return;
    }
    // 先隐藏聊天窗口
    if (m_chatWindow && m_chatWindow->isVisible()) {
        m_chatWindow->hide();
        // 使用QTimer确保窗口隐藏完成后再开始截图
        QTimer::singleShot(500, this, [this]() {
            startScreenshotInternal();
        });
    } else {
        // 如果窗口已经隐藏或不存在，直接开始截图
        startScreenshotInternal();
    }
}

void EAiExecutor::setInputFileSize(int inputFileSize)
{
    m_inputFileSize = inputFileSize;
}

void EAiExecutor::startScreenshotInternal()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    // 监听Screenshot信号
    connection.connect("com.deepin.Screenshot", "/com/deepin/Screenshot", "com.deepin.Screenshot", "CustomDone", this, SLOT(onScreenshotCustomDone(QString)));

    QDBusInterface screenshotInterface(
        "com.deepin.Screenshot",
        "/com/deepin/Screenshot",
        "com.deepin.Screenshot"
        );

    QVariantMap options;
    options["showToolBar"] = true;

    // 发起异步 DBus 调用
    QDBusPendingCall pendingCall = screenshotInterface.asyncCall(
        "CustomScreenshot",
        options
        );

    // 创建 watcher 来监听异步调用结果
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &EAiExecutor::onScreenshotCallFinished);
}

void EAiExecutor::setConversationMode(int mode)
{
    ConversionMode conversionMode = static_cast<ConversionMode>(mode);
    // tid:1001600009 event:private_chat_clicked
    if (m_conversionMode != conversionMode && conversionMode == ConversionMode::Private) {
        ReportIns()->writeEvent(report::PrivateChatClickedPoint().assemblingData());
    }
    m_conversionMode = conversionMode;
}

void EAiExecutor::showKnowledgeBaseErrorDialog(int type)
{
    m_chatWindow->showKnowledgeBaseErrorDialog(type, PLUGINSNAME);
}

/**
 * @brief EAiExecutor::loadAiFAQ
 *      en_US stand for enghish
 *      zh_CN stand for Chinese (Traditional&Simplified)
 */
void EAiExecutor::loadAiFAQ()
{
    qCInfo(logAIGUI) << "Loading AI FAQ files";
    m_sysLang = QLocale::system();

    QString faqPattern(":/assets/faq/AI-FAQ-%1.json");
    QString assistantFaqPattern(":/assets/faq/ASSISTANT-UOS-FAQ-%1.json");

    if (Util::isCommunity()) {
        assistantFaqPattern = ":/assets/faq/ASSISTANT-DEEPIN-FAQ-%1.json";
    }

    //Non-English country default use chinse FAQ file.
    QString FAQFilePath = QString(faqPattern).arg("zh_CN");
    QString AssistantFAQFilePath = QString(assistantFaqPattern).arg("zh_CN");

    if (QLocale::Chinese != m_sysLang.language() || QLocale::SimplifiedChineseScript != m_sysLang.script()) {
        QString path = QString(faqPattern).arg("en_US");
        if (QFileInfo(path).exists()) {
            FAQFilePath = path;
        }
        path = QString(assistantFaqPattern).arg("en_US");
        if (QFileInfo(path).exists()) {
            AssistantFAQFilePath = path;
        }
    }

    // 读取JSON文件
    QFile file(FAQFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(logAIGUI) << "Failed to load AI FAQ file";
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qCWarning(logAIGUI) << "AI FAQ file format error";
        return;
    }

    QJsonArray jsonArray = jsonDoc.array();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "icons/";
            m_aiFAQ.append(faqObject);
        }
    }

    //Shuffle the FAQ array
    /*QRandomGenerator randomGenerator(QDateTime::currentSecsSinceEpoch());
    int arraySize = m_aiFAQ.size();
    for (int i = arraySize - 1; i > 0; --i) {
        int j = randomGenerator.bounded(i + 1);
        qSwap(m_aiFAQ[i], m_aiFAQ[j]);
    }*/


    // 玩机助手
    QFile AssistantFaqfile(AssistantFAQFilePath);
    if (!AssistantFaqfile.open(QIODevice::ReadOnly)) {
        qCWarning(logAIGUI) << "Failed to load Assistant FAQ file.";
        return;
    }
    jsonData = AssistantFaqfile.readAll();
    AssistantFaqfile.close();

    jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qCWarning(logAIGUI) << "Assistant FAQ file format error!";
        return;
    }

    jsonArray = jsonDoc.array();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "icons/";
            m_assistantFAQ.append(faqObject);
        }
    }
    
    // 写作助手
    jsonArray = m_faqInitTool.createWritingFAQArray();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "icons/";
            m_aiWritingFAQ.append(faqObject);
        }
    }

    // 文本处理助手
    jsonArray = m_faqInitTool.createTextProcessingFAQArray();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "icons/";
            m_aiTextProcessingFAQ.append(faqObject);
        }
    }

    // 翻译助手
    jsonArray = m_faqInitTool.createTranslationFAQArray();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "icons/";
            m_aiTranslationFAQ.append(faqObject);
        }
    }

    //个人知识库问题加载
    QString lang = "zh_CN";
    if (QLocale::Chinese != m_sysLang.language() || QLocale::SimplifiedChineseScript != m_sysLang.script()) {
        lang = "en_US";
    }
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QFile personalFaqfile(dataDir.path() + QString("/personal-faq-%1.json").arg(lang));
    if (!personalFaqfile.open(QIODevice::ReadOnly)) {
        qCWarning(logAIGUI) << "Failed to load Personal FAQ file.";
        return;
    }
    jsonData = personalFaqfile.readAll();
    personalFaqfile.close();

    jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qCWarning(logAIGUI) << "Personal FAQ file format error!";
        return;
    }

    jsonArray = jsonDoc.array();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "icons/";
            m_persKnowledgeBaseFAQ.append(faqObject);
        }
    }
    qCInfo(logAIGUI) << "AI FAQ files loaded successfully";
}

void EAiExecutor::initAudioRecord()
{
    connect(AudioControler::instance(), &AudioControler::textReceived,
            this, &EAiExecutor::audioASRInComing);
    connect(AudioControler::instance(), &AudioControler::recordError,
            this, &EAiExecutor::audioASRError);

    connect(AudioControler::instance(), &AudioControler::playerError,
            this, &EAiExecutor::playTTSError);
    connect(AudioControler::instance(), &AudioControler::playTextFinished,
            this, &EAiExecutor::playTTSFinished);

    connect(AudioControler::instance(), &AudioControler::playDeviceChanged,
            this, &EAiExecutor::audioOutputDeviceChanged);
    connect(AudioControler::instance(), &AudioControler::recordDeviceChange,
            this, &EAiExecutor::audioInputDeviceChange);
    connect(AudioControler::instance(), &AudioControler::levelUpdated,
            this, &EAiExecutor::audioSampleLevel);
}

void EAiExecutor::initNetworkMonitor()
{
    qCInfo(logAIGUI) << "Initializing network monitor";
    connect(&NetworkMonitor::getInstance(), &NetworkMonitor::stateChanged,
            this, &EAiExecutor::netStateChanged);
}

QString EAiExecutor::getAssistantFunctions(int type)
{
    QString functionFilePath;
    QJsonArray functions;
    // 根据助手类型选择对应的函数定义文件
    switch (type) {
    case AssistantType::AI_WRITING:
        functions = m_faqInitTool.createWritingFunctionArray();
        break;
    case AssistantType::AI_TEXT_PROCESSING:
        functions = m_faqInitTool.createTextProcessingFunctionArray();
        break;
    default:
        return "";
    }

    QJsonDocument doc;
    doc.setArray(functions);
    return QString::fromUtf8(doc.toJson());
}

QString EAiExecutor::getFunctionTemplate(int type, const QString &function, const QString &contain)
{
    QByteArray jsonData;
    
    // 根据助手类型选择对应的模板文件
    switch (type) {
    case AssistantType::AI_WRITING:
        jsonData = m_faqInitTool.createWritingFunctionTemplate();
        break;
    default:
        return "";
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject()) {
        qCWarning(logAIGUI) << "Invalid function template file format";
        return "";
    }
    
    QJsonObject templates = doc.object();
    if (!templates.contains(function)) {
        qCWarning(logAIGUI) << "Function template not found:" << function;
        return "";
    }
    
    QJsonObject functionTemplate = templates[function].toObject();
    QString templateStr = functionTemplate["Template"].toString();
    QString defaultStr = functionTemplate["Default"].toString();
    
    // 如果contain为空，使用default值
    if (contain.isEmpty()) {
        return templateStr.arg(defaultStr);
    }
    
    return templateStr.arg(contain);
}
