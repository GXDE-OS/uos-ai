#include "eaiexecutor.h"
#include "eaistreamhandler.h"
#include "serverwrapper.h"
#include "audiocontroler.h"
#include "application.h"
#include "networkmonitor.h"

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
//#define AI_DEBUG

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

QString EAiExecutor::sendAiRequst(const QString &llmId, const QString &prompt, QSharedPointer<EAiCallback> callback)
{
    QString reqId("");

#ifdef AI_DEBUG
    qInfo() << "llmId=" << llmId << " prompt=" << prompt;
#endif

    //Make ai context
    QString aiCtx = makeAiReqContext(prompt);

    auto reply = m_aiProxy->requestChatText(llmId, aiCtx, m_temperature, callback->isStreamMode());

    QStringList params(reply.second);
    reqId = params.value(0);

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
    auto accountInfos = m_aiProxy->queryLLMAccountList();

    return accountInfos;
}

bool EAiExecutor::setCurrentLLMAccountId(const QString &id)
{
    bool fSetOk = m_aiProxy->setCurrentLLMAccountId(id);

    return fSetOk;
}

void EAiExecutor::launchLLMConfigWindow(bool showAddllmPage)
{
    m_aiProxy->launchLLMUiPage(showAddllmPage);
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
    int startPos =  m_aiFAQ.size() > m_limitAQCount ?
                    randomGenerator.bounded(m_aiFAQ.size() - m_limitAQCount)
                    : 0;

    for (int i = 0; i < m_limitAQCount; i++) {
        faqArray.append(m_aiFAQ[i + startPos]);
    }

    QJsonDocument doc(faqArray);

    return QString(doc.toJson(QJsonDocument::Compact));;
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

bool EAiExecutor::startRecorder()
{
    return AudioControler::instance()->startRecorder();
}

bool EAiExecutor::stopRecorder()
{
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

QString EAiExecutor::makeAiReqContext(const QString &question)
{
    QJsonDocument aiCtxDoc;

    QJsonArray ctxArray;

    QString questMd5 = QCryptographicHash::hash(
                           question.toUtf8(),
                           QCryptographicHash::Md5
                       ).toHex();
    //Modify 2020-11-18
    //  refRec.questHash == questMd5 means this
    //retry requset, need skip the record.

    if (m_chatHistories.size() > 0) {
        auto &refRec = m_chatHistories.last();

        int recCount = m_chatHistories.size();

        //Skip the current retury record
        if (refRec.questHash == questMd5) {
            recCount--;
        }

        //Add all histories to context array
        for (int index = 0; index < recCount; index++) {
            processAiRecord(m_chatHistories.at(index), ctxArray);
        }
    }

    //Add Question to context array
    QJsonObject questObj;

    questObj["role"] = "user";
    questObj["content"] = question;
    ctxArray.append(questObj);

    aiCtxDoc.setArray(ctxArray);
    QString ctxJson(aiCtxDoc.toJson(QJsonDocument::Compact));

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
        m_chatHistories.push_back(rec);

    } else {
        //handle retry histories.
        auto &recRef = m_chatHistories.last();

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

    if (m_chatHistories.size() > 0) {
        if (lastRec) {

            auto lastRec = m_chatHistories.last();

            makeJsonFromChatRecord(lastRec, historyArray);
        } else {

            foreach (auto rec, m_chatHistories) {
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
    m_chatHistories.clear();
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

void EAiExecutor::onCommandsReceived(const QVariantMap &commands)
{
}

void EAiExecutor::onChatTextReceived(const QString &callId, const QString &chatText)
{
#ifdef AI_DEBUG
    qInfo() << "callId=" << callId << " chatText=" << chatText;
#endif
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
}

void EAiExecutor::onChatError(const QString &id, int code, const QString &errorString)
{
    qInfo() << " callId=" << id
            << " error=" << code
            << " errorString=" << errorString;

    //Reverse the error code if positive
    if (code > 0) {
        code = -code;
    }

    auto reqIter = m_callQueue.find(id);

    if (reqIter != m_callQueue.end()) {
        reqIter->get()->notify(errorString, code);

        m_callQueue.erase(reqIter);
    }
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

    //Non-English country default use chinse FAQ file.
    QString FAQFilePath = QString(faqPattern).arg("zh_CN");

    if (QLocale::Chinese != m_sysLang.language() || QLocale::SimplifiedChineseScript != m_sysLang.script()) {
        QString path = QString(faqPattern).arg("en_US");
        if (QFileInfo(path).exists()) {
            FAQFilePath = path;
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
            m_aiFAQ.append(faqObject);
        }
    }

    //Shuffle the FAQ array
    QRandomGenerator randomGenerator(QDateTime::currentSecsSinceEpoch());
    int arraySize = m_aiFAQ.size();
    for (int i = arraySize - 1; i > 0; --i) {
        int j = randomGenerator.bounded(i + 1);
        qSwap(m_aiFAQ[i], m_aiFAQ[j]);
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
