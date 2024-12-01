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
#include "private/welcomedialog.h"

#include <QDBusConnection>
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

#include <DFileDialog>

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

EAiExecutor::EAiExecutor(QObject *parent)
    : QObject(parent)
{
    initAiSession();

    initAudioRecord();

    initNetworkMonitor();

    loadAiFAQ();

    QStringList docFiles = EmbeddingServer::getInstance().getDocFiles();
    if (!docFiles.isEmpty()) {
        m_knowledgeBaseExist = true;
    }
    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::addToServerStatusChanged, this, &EAiExecutor::onAddToServerStatusChanged);
    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::indexDeleted, this, &EAiExecutor::onIndexDeleted);
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

        //Init TTP
        connect(m_aiProxy.get(), &Session::chatAction,
                this, &EAiExecutor::chatConversationType);
        connect(m_aiProxy.get(), &Session::text2ImageReceived
                , this, &EAiExecutor::onTextToPictureData);
    }

    return !m_aiProxy.isNull();
}

QString EAiExecutor::sendAiRequst(const QString &llmId, const QString &prompt, QSharedPointer<EAiCallback> callback, bool isFAQGeneration)
{
    QString reqId("");

#ifdef AI_DEBUG
    qInfo() << "llmId=" << llmId << " prompt=" << prompt;
#endif

    if (!isFAQGeneration && m_aiProxy->currentAssistantType() == AssistantType::PLUGIN_ASSISTANT
            && !DbWrapper::localDbWrapper().getAICopilotIsOpen()) {
        WelcomeDialog welcomeDlg(nullptr, true);
        int ret = welcomeDlg.exec();
        if (ret > 0) {
            DbWrapper::localDbWrapper().updateAICopilot(true);
        } else {
            return "";
        }
    }

    //Make ai context
    QString aiCtx = makeAiReqContext(prompt, isFAQGeneration);

    QPair<AIServer::ErrorType, QStringList> reply;
    if (m_aiProxy->currentAssistantType() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT && !m_knowledgeBaseExist) {
        QString uuid = QUuid::createUuid().toString(QUuid::Id128);
        QStringList list;
        list << uuid;
        list << tr("The Personal Knowledge Assistant can only be used after configuring the knowledge base.");
        reply = qMakePair(AIServer::PersonalBaseNotExist, list);
    } else {
        reply = m_aiProxy->requestChatText(llmId, aiCtx, m_temperature, callback->isStreamMode(), isFAQGeneration);
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
    } else {
        //Handle the no account error code
        int errCode = reply.first;
        QString errInfo(reply.second.value(1));

        //the notify should be delay after the request is return.
        QTimer::singleShot(30, this, [callback, errInfo, errCode] {
            callback->notify(errInfo, -errCode);
        });

        qInfo() << "reqId=" << reqId
                << " error=" << errCode
                << " errorString=" << errInfo;
    }

    if (!isFAQGeneration) {
        m_isAnswering = true;
        m_chatWindow->setVoiceConversationDisabled(true);
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
            qInfo() << "Stop request:"
                    << " aiProxy=" << aiProxy
                    << " callId=" << reqiter.key()
                    << " isStreamMode=" << reqiter->get()->isStreamMode();

            reqiter->get()->setOwner(nullptr);
        }
    }
}

void EAiExecutor::cancelAiRequst(const QString &id)
{
    qInfo() << "Cancel the request:" << id;

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

QString EAiExecutor::queryLLMAccountList()
{
    auto accountInfos = m_aiProxy->queryLLMAccountListWithRole();

    return accountInfos;
}

bool EAiExecutor::setCurrentLLMAccountId(const QString &id)
{
    bool fSetOk = m_aiProxy->setCurrentLLMAccountId(id);

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
    bool fSetOk = m_aiProxy->setCurrentAssistantId(id);

    if (fSetOk)
        m_chatWindow->setHasChatHistory(m_assistantChatHistories[m_aiProxy->currentAssistantId()].size() > 0);


    return fSetOk;
}

QString EAiExecutor::currentAssistantName()
{
    return m_aiProxy->currentAssistantDisplayName();
}

AssistantType EAiExecutor::currentAssistantType()
{
    return m_aiProxy->currentAssistantType();
}

void EAiExecutor::launchLLMConfigWindow(bool showAddllmPage, bool onlyUseAgreement)
{
    m_aiProxy->launchLLMUiPage(showAddllmPage, onlyUseAgreement);
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

    if (!m_isFAQGenerating && !currentLLMAccountId().isEmpty() && m_knowledgeBaseExist && m_persKnowledgeBaseFAQ .isEmpty()) {
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
        if (faqData.isEmpty())
            return "";

        return QString(faqData);
    }
    default:
        return "";

    }
}

void EAiExecutor::setChatWindow(ChatWindow *window)
{
    m_chatWindow = window;
}

void EAiExecutor::showToast(const QString &messge)
{
    if (nullptr != m_chatWindow) {
        m_chatWindow->showToast(messge);
    } else {
        qWarning() << "Invalid chatWindow parameter:";
    }
}

void EAiExecutor::closeChatWindow()
{
    if (nullptr != m_chatWindow) {
        m_chatWindow->closeWindow();
    } else {
        qWarning() << "Invalid chatWindow parameter:";
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

    answerObj["role"] = "assistant";

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

QString EAiExecutor::makeAiReqContext(const QString &question, bool isFAQGeneration)
{
    QJsonDocument aiCtxDoc;
    QJsonArray ctxArray;
    QString context;

    if (isFAQGeneration) {
        QStringList knowleadge;
        QStringList docContents = EmbeddingServer::getInstance().getDocContent();
        QString chunk;
        int it = 1;
        for (const QString &docContent : docContents) {
            chunk += QString::number(it) + ":" + docContent + "\n";
            it++;
        }
        QString promptTemplate ="背景知识：\n" \
                                "%1\n" \
                                "请根据背景知识生成30个问题，%2输出问题列表，输出开头固定为\"问题列表：\"";
        if (QLocale::Chinese != m_sysLang.language() || QLocale::SimplifiedChineseScript != m_sysLang.script()) {
            context = promptTemplate.arg(chunk).arg("并翻译成英文，");
        } else {
            context = promptTemplate.arg(chunk).arg("");
        }
        qInfo() << context;
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
                QString promptTemplate = "请根据以下背景知识：\n" \
                                         "%1\n\n" \
                                         "回答问题：%2\n" \
                                         "如果背景知识中没有问题所需的答案，输出答案的开头固定为\"在您的知识库中，未找到相关的信息\"；" \
                                         "如果背景知识中有问题所需的答案，输出答案的开头固定为\"根据您的知识库\"";
                context = promptTemplate.arg(knowleadge).arg(question);
            }
            qInfo() << context;
        } else if (m_aiProxy->currentAssistantType() == AssistantType::UOS_SYSTEM_ASSISTANT
                   || m_aiProxy->currentAssistantType() == AssistantType::DEEPIN_SYSTEM_ASSISTANT) {
            QStringList result = EmbeddingServer::getInstance().searchVecor(question, topK, m_aiProxy->currentAssistantType());
            QString knowleadge;
            for (QString text : result) {
                knowleadge += text;
            }
            QString promptTemplate = "请根据以下背景知识：\n" \
                                     "%1\n\n" \
                                     "回答问题：%2\n" \
                                     "如果背景知识中没有问题所需的答案，只输出信息：\"嗯...问到我了，但我会努力去学习这个问题，等我学会了，就能为你解答了。\"";
            context = promptTemplate.arg(knowleadge).arg(question);
            qInfo() << context;
        } else {
            context = question;
        }
    }

    QString questMd5 = QCryptographicHash::hash(
                           context.toUtf8(),
                           QCryptographicHash::Md5
                       ).toHex();

    if (isKnowAssistantType())
        // 知识库角色只做问题的校验，不包含背景知识
        questMd5 = QCryptographicHash::hash(
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
    qInfo() << "AI Request Content: " << ctxJson;
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
                qWarning() << "Exception data record:" << rec;
            }
        } else {
            qWarning() << "Unexpected data record:" << rec;
        }
    }
}

bool EAiExecutor::isKnowAssistantType()
{
    return m_aiProxy->currentAssistantType() == AssistantType::UOS_SYSTEM_ASSISTANT
            || m_aiProxy->currentAssistantType() == AssistantType::DEEPIN_SYSTEM_ASSISTANT
            || m_aiProxy->currentAssistantType() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT;
}

void EAiExecutor::logChatRecord(const QString &reqId,
                                const QString &question,
                                const QStringList &answer,
                                bool isRetry,
                                int err,
                                const QString &errorInfo,
                                int actionType,
                                const QString &llmIcon,
                                const QString &llmName)
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
    //BCS these records are meaning less to LLM.
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
                qInfo() << "Quest:" << question
                        << " Update type from:" << recRef.actType
                        << " to " << ChatAction(actionType);

                recRef.actType = ChatAction(actionType);
            }

            recRef.replys.push_back(c);
        } else {
            qWarning() << "Drop retry for different quest:"
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

bool EAiExecutor::saveImageAs(const QString &filePath)
{
    qInfo() << " saveImageAs->" << filePath;

    QFileInfo imageInfo(filePath);

    bool saveOK = false;

    if (imageInfo.exists()) {
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

        QString imageFilePath = DFileDialog::getSaveFileName(
                                    m_chatWindow,
                                    tr("Export Image As"),
                                    picturesPath,
                                    filter,
                                    &selectFilter);

        //Check if path selected
        if (!imageFilePath.isEmpty()) {
            QImageReader imageReader(imageInfo.filePath());
            QImage inputImage = imageReader.read();

            if (!inputImage.isNull()) {
                QString fmtSuffix
                    = formatSuffix.at(supportedFormats.indexOf(selectFilter));

                // Save the image as a selected format file
                QImageWriter imageWriter(imageFilePath);
                imageWriter.setFormat(fmtSuffix.toUtf8()); // Specify the format


                if (imageWriter.write(inputImage)) {
                    saveOK = true;
                } else {
                    qWarning() << "imageWriter failed:"
                               << "path=" << imageFilePath
                               <<  " error=" << imageWriter.errorString();
                }
            } else {
                qWarning() << "imageReader failed:"
                           << "path=" << imageInfo.filePath()
                           <<  " error=" << imageReader.errorString();
            }
        }
    } else {
        qWarning() << __FUNCTION__
                   << " file can't find:" << filePath;
    }

    return saveOK;
}

bool EAiExecutor::previewImage(const QString &filePath)
{
    qInfo() << " previewImage->" << filePath;

    QFileInfo imageInfo(filePath);

    bool openOK = false;

    if (imageInfo.exists()) {
        openOK = QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    } else {
        qWarning() << __FUNCTION__
                   << " file can't find:" << filePath;
    }

    return openOK;
}

void EAiExecutor::copyImg2Clipboard(const QString &filePath)
{
    qInfo() << " copyImg2Clipboard->" << filePath;

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
            qWarning() << __FUNCTION__
                       << " imageReader failed:"
                       << " path=" << imageInfo.filePath()
                       << " error=" << imageReader.errorString();
        }
    } else {
        qWarning() << __FUNCTION__
                   << " file can't find:" << filePath;
    }
}

bool EAiExecutor::isNetworkAvailable()
{
    return NetworkMonitor::getInstance().isOnline();
}

void EAiExecutor::personalFAQGenerate()
{
    if (m_knowledgeBaseExist) {
        QSharedPointer<EAiCallback> aiCallback;
        aiCallback.reset(new EAiCacheCallback(this));
        aiCallback->setDataMode(EAiCallback::DataMode::StreamMode);
        aiCallback->setNotifier("sigAiReplyStream");
        aiCallback->setOp(1);
        QString assistantId = m_aiProxy->queryAssistantIdByType(AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT);
        QString llmId = DbWrapper::localDbWrapper().queryLlmIdByAssistantId(assistantId);
        sendAiRequst(llmId, "1", aiCallback, true);
        m_isFAQGenerating = true;
    }
}

void EAiExecutor::onCommandsReceived(const QVariantMap &commands)
{
}

void EAiExecutor::onChatTextReceived(const QString &callId, const QString &chatText)
{
#ifdef AI_DEBUG
    qInfo() << "callId=" << callId << " chatText=" << chatText;
#endif

    if (m_faqId == callId) {
        m_isFAQGenerating = false;
        // 个人知识库问题生成结果
        QStringList list = chatText.split('\n', QString::SkipEmptyParts);
        QStringList faqList;
        /*foreach(QString line, list) {
            QRegularExpression regex("^(\\d+)(.*)"); // 匹配以数字开头，任意字符，换行结尾的字符串
            QRegularExpressionMatch match = regex.match(line);
            if (match.hasMatch()) {
                // 获取匹配结果中的第二个捕获组（去掉开头的数字）
                QString captured = match.captured(2);
                captured = captured.remove(QRegExp("^[^\\w]+"));
                faqList.append(captured);
            }
        }*/
        for (int i = 1; i < list.size(); i++) {
            QString faq = list[i].trimmed();
            faq.remove(QRegExp("^(\\d+)(\\.*)"));
            faqList.append(faq);
        }
        if (faqList.size() >= 6) {
            QJsonArray jsonArr;
            m_persKnowledgeBaseFAQ.clear();
            for (int i = 0; i < faqList.size(); i++) {
                QJsonObject faqObject = {
                    {"iconName", QString::number(i % 10 + 1) + "-question-personal"},
                    {"Question", faqList[i]},
                    {"iconPrefix", "icons/"}};
                m_persKnowledgeBaseFAQ.append(faqObject);
                jsonArr.append(faqObject);
            }

            emit knowledgeBaseFAQGenFinished();

            QJsonDocument jsonDoc(jsonArr);
            QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
            if (dataDir.exists() == false) {
                dataDir.mkpath(dataDir.path());
            }
            QString lang = "zh_CN";
            if (QLocale::Chinese != m_sysLang.language() || QLocale::SimplifiedChineseScript != m_sysLang.script()) {
                lang = "en_US";
            }
            QFile file(dataDir.path() + QString("/personal-faq-%1.json").arg(lang));
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.write(jsonDoc.toJson());
                file.close();
            }
        }

        return;
    }

    auto reqIter = m_callQueue.find(callId);

    if (reqIter != m_callQueue.end()) {
        //Notify the calling owner the result.
        //We only send the full cached result in
        //cache mode
        if (reqIter->get()->isStreamMode()) {
            reqIter->get()->notify(QString(""), EAiStreamHandler::StreamEnd);
        } else {
            reqIter->get()->notify(chatText, 0);
        }

        m_callQueue.erase(reqIter);
    }
    m_isAnswering = false;
    m_chatWindow->setVoiceConversationDisabled(false);
}

void EAiExecutor::onChatError(const QString &id, int code, const QString &errorString)
{
    qInfo() << " callId=" << id
            << " error=" << code
            << " errorString=" << errorString;

    if (code == AIServer::NetworkError)  // 网络异常
        emit netStateChanged(false);

    //Reverse the error code if positive
    if (code > 0) {
        code = -code;
    }

    auto reqIter = m_callQueue.find(id);

    if (reqIter != m_callQueue.end()) {
        reqIter->get()->notify(errorString, code);

        m_callQueue.erase(reqIter);
    }

    m_isAnswering = false;
    m_chatWindow->setVoiceConversationDisabled(false);
}

void EAiExecutor::onTextToPictureData(const QString &id, const QList<QByteArray> &imageData)
{
    QStringList imagePaths;

    if (m_ttpDir.isValid()) {
        foreach (auto p, imageData) {
            auto time_point = std::chrono::high_resolution_clock::now();
            auto nanoseconds = std::chrono::time_point_cast<std::chrono::nanoseconds>(time_point).time_since_epoch().count();

            //FileName=<Id_timestamp precise in nano seconds>
            QString imageFileName = QString("%1/%2_%3")
                                    .arg(m_ttpDir.path())
                                    .arg(id)
                                    .arg(nanoseconds);
            QFile file(imageFileName);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(p);
                file.close();

                imagePaths << imageFileName;
            }
        }
    } else {
        qWarning() << "Create TTP directory error:" << m_ttpDir.errorString();
    }

    emit textToPictureFinish(id, imagePaths);
    m_isAnswering = false;
    m_chatWindow->setVoiceConversationDisabled(false);
}

void EAiExecutor::onAddToServerStatusChanged(const QStringList &files, int status)
{
    if (files.isEmpty())
        return;

    if(status == 1 && !m_knowledgeBaseExist) {
        m_knowledgeBaseExist = true;
    }

    emit knowledgeBaseExistChanged(m_knowledgeBaseExist);

    //personalFAQGenerate();
}

void EAiExecutor::onIndexDeleted(const QStringList &files)
{
    //qDebug() << "IndexDeleted, " << files;
    if (files.isEmpty())
        return;

    QStringList docFiles = EmbeddingServer::getInstance().getDocFiles();
    if (docFiles.isEmpty()) {
        m_knowledgeBaseExist = false;
        m_persKnowledgeBaseFAQ.clear();
        m_faqId = "";

        emit knowledgeBaseExistChanged(m_knowledgeBaseExist);

        QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        QString lang = "zh_CN";
        if (QLocale::Chinese != m_sysLang.language() || QLocale::SimplifiedChineseScript != m_sysLang.script()) {
            lang = "en_US";
        }
        QFile file(dataDir.path() + QString("/personal-faq-%1.json").arg(lang));
        if (file.exists()) {
            // 删除文件
            bool success = file.remove();
            if (success) {
                qDebug() << "personal faq file deleted successfully.";
            } else {
                qDebug() << "Failed to delete personal faq file.";
            }
        }
    }

    personalFAQGenerate();
}

/**
 * @brief EAiExecutor::loadAiFAQ
 *      en_US stand for enghish
 *      zh_CN stand for Chinese (Traditional&Simplified)
 */
void EAiExecutor::loadAiFAQ()
{
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
        qWarning() << "Failed to load AI FAQ file.";
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qWarning() << "AI FAQ file format error!";
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
        qWarning() << "Failed to load Assistant FAQ file.";
        return;
    }
    jsonData = AssistantFaqfile.readAll();
    AssistantFaqfile.close();

    jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qWarning() << "Assistant FAQ file format error!";
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

    //个人知识库问题加载
    QString lang = "zh_CN";
    if (QLocale::Chinese != m_sysLang.language() || QLocale::SimplifiedChineseScript != m_sysLang.script()) {
        lang = "en_US";
    }
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QFile personalFaqfile(dataDir.path() + QString("/personal-faq-%1.json").arg(lang));
    if (!personalFaqfile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to load Personal FAQ file.";
        return;
    }
    jsonData = personalFaqfile.readAll();
    personalFaqfile.close();

    jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qWarning() << "Personal FAQ file format error!";
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
    connect(&NetworkMonitor::getInstance(), &NetworkMonitor::stateChanged,
            this, &EAiExecutor::netStateChanged);
}
