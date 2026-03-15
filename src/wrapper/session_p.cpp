#include "session_p.h"
#include "session.h"
#include "timereventloop.h"
#include "appsocketserver.h"
//#include "llmutils.h"
#include "dbwrapper.h"
#include "threadtaskmana.h"
#include "uossimplelog.h"
#include "uosfreeaccounts.h"
#include "functionhandler.h"
#include "chatfixedresponse.h"
#include "servercodetranslation.h"
#include "conversation.h"
#include "text2imagehandler.h"
#include "networkmonitor.h"
#include "generateprompt.h"
#include "utils/globalfilewatcher.h"
#include "agentfactory.h"
#include "embeddingserver.h"
#include "global.h"
#include "global_define.h"
#include "mcpconfigsyncer.h"

#include <QtConcurrent>
#include <QImage>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logWrapper)

using namespace uos_ai;

SessionPrivate::SessionPrivate(Session *session, const QString &appId)
    : m_appId(appId)
    , m_q(session)
{
    
    LLMVendor()->initExternal();

    {
        QString assistantId = DbWrapper::localDbWrapper().queryCurAssistantIdByAppId(appId);
        checkUpdateAssistantAccount(assistantId);
    }

    {
        QString llmId = DbWrapper::localDbWrapper().queryLlmIdByAssistantId(m_assistantProxy.id);
        auto supLLM = LLMVendor()->queryServerAccountByRole(m_assistantProxy);

        if (llmId.isEmpty() && m_assistantProxy.type == UOS_AI) {
            qCDebug(logWrapper) << "No LLM ID found for UOS AI assistant, trying app's configured model";
            llmId = DbWrapper::localDbWrapper().queryCurLlmIdByAppId(appId);
            if (!supLLM.isEmpty()) {
                const auto it = std::find_if(supLLM.begin(), supLLM.end(), [llmId](const LLMServerProxy &account){
                    return account.id == llmId;
                });

                if (it == supLLM.end()) {
                    qCWarning(logWrapper) << "Configured LLM" << llmId << "not supported for UOS AI assistant";
                    llmId.clear();
                }
            } else {
                llmId.clear();
            }
        }

        m_appServerProxy = LLMVendor()->queryValidServerAccount(llmId);
        if (!m_appServerProxy.isValid()) {
            if (!supLLM.isEmpty()) {
                qCDebug(logWrapper) << "Using first available LLM for assistant";
                m_appServerProxy = supLLM.first();
            } else {
                qCWarning(logWrapper) << "No LLM available for assistant:" 
                                    << m_assistantProxy.id 
                                    << "name:" << m_assistantProxy.displayName;
            }
        }
    }

    FileWatcher *watcher = new FileWatcher(this);
    watcher->addPath(FunctionHandler::functionPluginPath());
    qCDebug(logWrapper) << "Watching function plugin path:" << FunctionHandler::functionPluginPath();

    connect(watcher, &FileWatcher::directoryChanged, this, [=](const QString &path) {
        qCDebug(logWrapper) << "Function plugin directory changed:" << path;
        m_needQueryFunctions = true;
    });

    connect(watcher, &FileWatcher::fileChanged, this, [=](const QString &path) {
        qCDebug(logWrapper) << "Function plugin file changed:" << path;
        m_needQueryFunctions = true;
    });

    connect(LLMVendor(), &LLMServiceVendor::agentChanged, m_q, &Session::assistantListChanged);
}

SessionPrivate::~SessionPrivate()
{
    for (const QString &id : m_runTaskIds) {
        LLMThreadTaskMana::instance()->requestTaskFinished(id);
    }
}

void SessionPrivate::cancelRequestTask(const QString &id)
{
    qCDebug(logWrapper) << "Cancelling request task:" << id;
    m_runTaskIds.removeAll(id);
    LLMThreadTaskMana::instance()->cancelRequestTask(id);
}

bool SessionPrivate::handleRequestError(QSharedPointer<LLM> copilot, const QString &uuid)
{
    int errorCode;
    QString errorString;

    if (!NetworkMonitor::getInstance().isOnline()) {
        qCWarning(logWrapper) << "Network offline, request aborted:" << uuid;
        errorCode = AIServer::NetworkError;
        errorString = ServerCodeTranslation::serverCodeTranslation(errorCode, "");
    } else {
        errorCode = copilot->lastError();
        errorString = copilot->lastErrorString();
        qCWarning(logWrapper) << "Request error - uuid:" << uuid << "code:" << errorCode << "message:" << errorString;
    }

    QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, errorCode), Q_ARG(QString, errorString));
    return true;
}

bool SessionPrivate::supportFunctionCall(const LLMServerProxy &account)
{
    if (qApp->applicationName() != m_appId) {
        qCDebug(logWrapper) << "Function call not supported for app:" << qApp->applicationName();
        return false;
    }

    if (LLMServerProxy::llmManufacturer(account.model) == ModelManufacturer::BAIDU_WXQF) {
        qCDebug(logWrapper) << "Function call supported for Baidu WXQF model";
        return true;
    }

//    if (account.model == LOCAL_YOURONG_1_5B || account.model== LOCAL_YOURONG_7B)
//        return true;

//    if (account.model == DeepSeek_Uos_Free)
//        return true;

    return false;
}

bool SessionPrivate::handleAiServerRequest(QSharedPointer<LLM> copilot, const QString &uuid
                                           , QJsonObject &response, const QString &conversation
                                           , const QJsonArray &functions, const LLMServerProxy &llmAccount)
{
#ifdef DEBUG_LOG
    qCDebug(logWrapper) << "Handling AI server request for" << uuid 
                       << "model:" << LLMServerProxy::llmName(llmAccount.model);
#endif

    QString maybeResponse = ChatFixedResponse::checkRequestText(conversation);
    if (!maybeResponse.isEmpty()) {
        qCDebug(logWrapper) << "Using fixed response for request:" << uuid;
        response["content"] = maybeResponse;

        // 给点延时，直接返回，接收端可能还没来得及连接
        TimerEventLoop loop;
        loop.setTimeout(20);
        loop.exec();

        copilot->textChainContent(maybeResponse);
        return true;
    }

    for (int i = 0; i < 3; i++) {
        response = copilot->predict(conversation, functions);

        // 速率被限制了，尝试请求下
        if (copilot->lastError() != AIServer::ServerRateLimitError && copilot->lastError() != AIServer::TimeoutError) {
            break;
        }

        qCWarning(logWrapper) << "Rate limit or timeout for request" << uuid 
                            << "attempt:" << i + 1;
        int timeout = 500;

        TimerEventLoop loop;
        connect(copilot.data(), &LLM::aborted, &loop, &TimerEventLoop::quit);
        loop.setTimeout(timeout);
        loop.exec();
        disconnect(copilot.data(), &LLM::aborted, &loop, &TimerEventLoop::quit);
    }

#ifdef DEBUG_LOG
    qCWarning(logWrapper) << "ChatSeesion Response = " << response;
#endif

    // 如果结束了，只有一种可能，此对象被销毁了，销毁了就不要继续下面的流程了
    if (LLMThreadTaskMana::instance()->isFinished(uuid)) {
        qCDebug(logWrapper) << "Request" << uuid << "was finished";
        return false;
    }

    if (copilot->lastError() != AIServer::NoError) {
        qCWarning(logWrapper) << "AI server request error for" << uuid 
                            << "code:" << copilot->lastError() 
                            << "message:" << copilot->lastErrorString()
                            << "is replied" << copilot->isReplied();
        handleRequestError(copilot, uuid);
        return false;
    }

    return true;
}

LLMServerProxy SessionPrivate::queryValidServerAccount(const QString &llmId)
{
    LLMServerProxy tmpLLMAccount = LLMVendor()->queryValidServerAccount(llmId);
    if (!tmpLLMAccount.isValid())
        tmpLLMAccount = m_appServerProxy;

    return tmpLLMAccount;
}

bool SessionPrivate::checkLLMAccountStatus(const QString &uuid, LLMServerProxy &llmAccount, int &errorType)
{
    qCDebug(logWrapper) << "Checking LLM account status for" << uuid 
                       << "model:" << LLMServerProxy::llmName(llmAccount.model);
                       
    // 免费账号判断有效期和次数
    if (llmAccount.type == ModelType::FREE_NORMAL || llmAccount.type == ModelType::FREE_KOL) {
        int available = 0;
        QString modelUrl;
        bool claimAgain = true;
        int errorCode = UosFreeAccounts::instance().getDeterAccountLegal(llmAccount.account.apiKey, available, modelUrl, claimAgain);
        if (!modelUrl.isEmpty() && llmAccount.model != WXQF_ERNIE_Bot_4)
            // 兼容旧版本添加的KOL bot_4账号
            llmAccount.url = modelUrl;
        auto serverCodeTranslation = [=](int errorType) {
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection,
                Q_ARG(QString, uuid),
                Q_ARG(int, errorType),
                Q_ARG(QString, ServerCodeTranslation::serverCodeTranslation(errorType, "").toUtf8())
                );
        };

        if (errorCode != QNetworkReply::NoError) {
            qCWarning(logWrapper) << "Network error checking account status:" << errorCode;
            errorType = AIServer::NetworkError;
            serverCodeTranslation(errorType);
            return false;
        }
        switch (available) {
        case 1:
            qCWarning(logWrapper) << "Free account expired";
            errorType = AIServer::FREEACCOUNTEXPIRED;
            serverCodeTranslation(errorType);
            return false;
        case 2:
            qCWarning(logWrapper) << "Free account usage limit reached";
            errorType = AIServer::FREEACCOUNTUSAGELIMIT;
            serverCodeTranslation(errorType);
            return false;
        case 5:
            qCDebug(logWrapper) << "Free account chat usage limit";
            errorType = AIServer::FREEACCOUNTCHATUSAGELIMIT;
            serverCodeTranslation(errorType);

            // 发送可以获取额外额度
            if (!claimAgain)
                QMetaObject::invokeMethod(m_q, "sigClaimAgain", Qt::QueuedConnection, Q_ARG(bool, claimAgain));

            return false;
        case 6:
            qCDebug(logWrapper) << "Free account text2image usage limit";
            errorType = AIServer::FREEACCOUNTTEXT2IMAGEUSAGELIMIT;
            break;
        default:
            qCDebug(logWrapper) << "Account status check passed";
            break;
        }
    }
    return true;
}

QString SessionPrivate::handleLocalModel(QSharedPointer<LLM> copilot, const QString &uuid, const QString &conversation, const LLMServerProxy &llmAccount)
{
    qCDebug(logWrapper) << "Handling local model request for" << uuid;
    
    QString prompt = Conversation::conversationLastUserData(conversation);

    QList<QByteArray> imgData = copilot->text2Image(prompt, 4);
    QMetaObject::invokeMethod(m_q, "chatAction", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, ChatText2Image));

    if (copilot->lastError() != AIServer::NoError) {
        qCWarning(logWrapper) << "Local model error for" << uuid 
                            << "code:" << copilot->lastError() 
                            << "message:" << copilot->lastErrorString();
        handleRequestError(copilot, uuid);
        return uuid;
    }

    QMetaObject::invokeMethod(m_q, "text2ImageReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QList<QByteArray>, imgData));
    return uuid;
}

QPair<AIServer::ErrorType, QStringList> SessionPrivate::requestChatFunctionText(LLMServerProxy tmpLLMAccount, const QString &conversation, const QVariantHash &params)
{
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);
    qCInfo(logWrapper) << "Starting general request:" << uuid;
    
    if (!tmpLLMAccount.isValid()) {
        qCWarning(logWrapper) << "Invalid LLM account for request:" << uuid;
        QStringList reply;
        reply << uuid;
        reply << QCoreApplication::translate("SessionPrivate", "UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
        return qMakePair(AIServer::AccountInvalid, reply);
    }

    bool stream = params.value(PREDICT_PARAM_STREAM, false).toBool();
    qCDebug(logWrapper) << "Request" << uuid << "stream mode:" << stream;

    // 这里如果开启了管道，需要等管道完全开启才能返回
    QEventLoop eventloop;
    bool isLoopQuit = false;

    QFuture<QString> future = QtConcurrent::run(LLMThreadTaskMana::instance()->threadPool(tmpLLMAccount.id), [ =, &eventloop, &isLoopQuit]() {
        LLMServerProxy tmpLLMAccountLocal = tmpLLMAccount;

        // 免费账号需要解密
        LLMServerProxy tmpLLMAccountDecrypt = tmpLLMAccountLocal.decryptAccount();

        QSharedPointer<LLM> copilot = LLMVendor()->getCopilot(tmpLLMAccountDecrypt);
        if (copilot.isNull()) {
            qCWarning(logWrapper) << "Failed to get copilot instance - uuid:" << uuid;
            eventloop.quit();
            isLoopQuit = true;
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, AIServer::ContentAccessDenied), Q_ARG(QString, "Invalid LLM Account."));
            return uuid;
        }
        qCDebug(logWrapper) << "Initializing copilot for request:" << uuid;
        copilot->loadParams(params);
        copilot->setCreatedId(uuid);
        copilot->switchStream(stream);

        connect(m_q, &Session::executionAborted, copilot.data(), &LLM::aborted);

        m_runTaskIds << uuid;
        LLMThreadTaskMana::instance()->addRequestTask(uuid, copilot.toWeakRef());

        if (tmpLLMAccountDecrypt.model == LLMChatModel::LOCAL_TEXT2IMAGE) {
            qCInfo(logWrapper) << "Handling local text2image model - uuid:" << uuid;
            eventloop.quit();
            isLoopQuit = true;
            return handleLocalModel(copilot, uuid, conversation, tmpLLMAccountDecrypt);
        }

        AppSocketServer socketServer(uuid, params.value(PREDICT_PARAM_NOJSONOUTPUT, false).toBool());
        qCDebug(logWrapper) << "Socket server created - uuid:" << uuid << socketServer.outputWithJson();

        // 开启流格式了，才会开启管道
        if (stream) {
            socketServer.startServer();
            connect(copilot.data(), &LLM::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);
            eventloop.quit();
            isLoopQuit = true;
            qCDebug(logWrapper) << "Stream mode initialized - uuid:" << uuid;
        }

        // 免费账号判断有效期和次数
        int errorType = -1;
        if (!checkLLMAccountStatus(uuid, tmpLLMAccountLocal, errorType)) {
            qCWarning(logWrapper) << "Account status check failed - uuid:" << uuid 
                                << "error type:" << errorType;
            return uuid;
        }
        // 获取模型接口URL
        tmpLLMAccountDecrypt.url = tmpLLMAccountLocal.url;

        // 更新llm的账号
        copilot->updateAccount(tmpLLMAccountDecrypt);

        QJsonObject appFunctions;
        // functions 目前只开放给助手自己使用
        if (supportFunctionCall(tmpLLMAccountDecrypt))
            appFunctions = FunctionHandler::queryAppFunctions(m_needQueryFunctions);

        QJsonObject response;
        QJsonArray functions = FunctionHandler::functions(appFunctions);
        qCDebug(logWrapper) << "Functions prepared - uuid:" << uuid 
                           << "count:" << functions.size();

        if (!handleAiServerRequest(copilot, uuid, response, conversation, functions, tmpLLMAccountDecrypt)) {
            if (copilot->isReplied() && (copilot->lastError() == AIServer::OperationCanceledError))
                increaseAccountUsage(tmpLLMAccountDecrypt, ChatTextPlain);
            return uuid;
        }

        increaseAccountUsage(tmpLLMAccountDecrypt, ChatTextPlain);

        int chatAction = FunctionHandler::chatAction(response);
        qCDebug(logWrapper) << "Chat action determined - uuid:" << uuid 
                           << "action:" << chatAction;
        QMetaObject::invokeMethod(m_q, "chatAction", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, chatAction));

        if (chatAction & ChatText2Image) {
            qCInfo(logWrapper) << "Processing text2image request - uuid:" << uuid;
            if (errorType == AIServer::FREEACCOUNTTEXT2IMAGEUSAGELIMIT) {
                qCWarning(logWrapper) << "Text2image usage limit reached - uuid:" << uuid;
                QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection,
                    Q_ARG(QString, uuid),
                    Q_ARG(int, errorType),
                    Q_ARG(QString, ServerCodeTranslation::serverCodeTranslation(errorType, "").toUtf8())
                    );
                return uuid;
            }

            QJsonObject tmpResponse;
            disconnect(copilot.data(), &LLM::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);
            qCDebug(logWrapper) << "Stream disconnected for text2image - uuid:" << uuid;

            QString prompt = Text2ImageHandler::imagePrompt(response, conversation);
            if (!handleAiServerRequest(copilot, uuid, tmpResponse, prompt, QJsonArray(), tmpLLMAccountDecrypt))
                return uuid;

            QList<QByteArray> imgData = copilot->text2Image(tmpResponse.value("content").toString(), 1);//文生图 1张

            if (copilot->lastError() != AIServer::NoError) {
                qCWarning(logWrapper) << "Image generation failed - uuid:" << uuid 
                                    << "error:" << copilot->lastErrorString();
                handleRequestError(copilot, uuid);
                return uuid;
            }

            qCInfo(logWrapper) << "Image generation completed - uuid:" << uuid;
            QMetaObject::invokeMethod(m_q, "text2ImageReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QList<QByteArray>, imgData));

            increaseAccountUsage(tmpLLMAccountDecrypt, chatAction);
            return uuid;
        }

        if (chatAction & ChatAction::ChatFunctionCall) {
            qCInfo(logWrapper) << "Processing function call - uuid:" << uuid;
            QString directReply;
            const QJsonArray &functionCalls = FunctionHandler::functionCall(response, conversation, &directReply);
            if (directReply.isEmpty()) {
                qCDebug(logWrapper) << "No direct reply, sending function calls - uuid:" << uuid;
                QJsonDocument document(functionCalls);
                if (!handleAiServerRequest(copilot, uuid, response, document.toJson(QJsonDocument::Compact), functions, tmpLLMAccountDecrypt))
                    return uuid;
            } else {
                qCDebug(logWrapper) << "Using direct reply - uuid:" << uuid;
                response["content"] = directReply;
                copilot->textChainContent(directReply);
            }
        }

        // 正常大部分场景不会进这里, 如果socket写入的字节比总字节还少，有可能还没写完，稍微延迟
        TimerEventLoop oneloop;
        oneloop.setTimeout(100);
        oneloop.exec();

        qCInfo(logWrapper) << "Sending chat text response - uuid:" << uuid;
        QMetaObject::invokeMethod(m_q, "chatTextReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QString, response.value("content").toString()));

        qCInfo(logWrapper) << "Request processing completed - uuid:" << uuid;
        return uuid;
    });

    QFutureWatcher<QString> *futureWatcher = new QFutureWatcher<QString>(this);
    QObject::connect(futureWatcher, &QFutureWatcher<QString>::finished, this, &SessionPrivate::onRequestTaskFinished);
    futureWatcher->setFuture(future);

    QStringList reply;
    reply << uuid;
    reply << "chatTextReceived";

    // 开启了stream才需要等待管道完全开启
    // 这里是多线程，可能已经quit了，还没来及exec
    if (stream && !isLoopQuit) {
        eventloop.exec();
    }

    return qMakePair(AIServer::NoError, reply);
}

QPair<AIServer::ErrorType, QStringList> SessionPrivate::requestRag(LLMServerProxy tmpLLMAccount, const QString &conversation, const QVariantHash &params)
{
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);
    qCInfo(logWrapper) << "Starting rag request:" << uuid;

    if (!tmpLLMAccount.isValid()) {
        qCWarning(logWrapper) << "Invalid LLM account for request:" << uuid;
        QStringList reply;
        reply << uuid;
        reply << QCoreApplication::translate("SessionPrivate", "UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
        return qMakePair(AIServer::AccountInvalid, reply);
    }

    bool stream = params.value(PREDICT_PARAM_STREAM, false).toBool();
    qCDebug(logWrapper) << "Request" << uuid << "stream mode:" << stream;

    // 这里如果开启了管道，需要等管道完全开启才能返回
    QEventLoop eventloop;
    bool isLoopQuit = false;

    QFuture<QString> future = QtConcurrent::run(LLMThreadTaskMana::instance()->threadPool(tmpLLMAccount.id), [ =, &eventloop, &isLoopQuit]() {
        LLMServerProxy tmpLLMAccountLocal = tmpLLMAccount;

        // 免费账号需要解密
        LLMServerProxy tmpLLMAccountDecrypt = tmpLLMAccountLocal.decryptAccount();

        QSharedPointer<LLM> copilot = LLMVendor()->getCopilot(tmpLLMAccountDecrypt);
        if (copilot.isNull()) {
            qCWarning(logWrapper) << "Failed to get copilot instance - uuid:" << uuid;
            eventloop.quit();
            isLoopQuit = true;
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, AIServer::ContentAccessDenied), Q_ARG(QString, "Invalid LLM Account."));
            return uuid;
        }
        qCDebug(logWrapper) << "Initializing copilot for request:" << uuid;
        copilot->loadParams(params);
        copilot->setCreatedId(uuid);
        copilot->switchStream(stream);

        connect(m_q, &Session::executionAborted, copilot.data(), &LLM::aborted);

        m_runTaskIds << uuid;
        LLMThreadTaskMana::instance()->addRequestTask(uuid, copilot.toWeakRef());

        AppSocketServer socketServer(uuid, params.value(PREDICT_PARAM_NOJSONOUTPUT, false).toBool());
        qCDebug(logWrapper) << "Socket server created - uuid:" << uuid << socketServer.outputWithJson();

        // 开启流格式了，才会开启管道
        if (stream) {
            socketServer.startServer();
            connect(copilot.data(), &LLM::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);
            eventloop.quit();
            isLoopQuit = true;
            qCDebug(logWrapper) << "Stream mode initialized - uuid:" << uuid;
        }

        // 免费账号判断有效期和次数
        int errorType = -1;
        if (!checkLLMAccountStatus(uuid, tmpLLMAccountLocal, errorType)) {
            qCWarning(logWrapper) << "Account status check failed - uuid:" << uuid
                                << "error type:" << errorType;
            return uuid;
        }
        // 获取模型接口URL
        tmpLLMAccountDecrypt.url = tmpLLMAccountLocal.url;

        // 更新llm的账号
        copilot->updateAccount(tmpLLMAccountDecrypt);

        QString ragPrompt = ragPromptBuild(conversation);
        if (ragPrompt.isEmpty()) {
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, AIServer::UnknownContentError), Q_ARG(QString, tr("Something's wrong. Please try again later.")));
            return uuid;
        }
        QJsonObject response = copilot->predict(ragPrompt, QJsonArray());

        if (LLMThreadTaskMana::instance()->isFinished(uuid))
            return uuid;

        if (copilot->lastError() != AIServer::NoError) {
            if (copilot->isReplied() && (copilot->lastError() == AIServer::OperationCanceledError))
                increaseAccountUsage(tmpLLMAccountDecrypt, ChatTextPlain);
            qCWarning(logWrapper) << "Chat error - uuid:" << uuid
                                << "code:" << copilot->lastError()
                                << "message:" << copilot->lastErrorString();
            handleRequestError(copilot, uuid);
            return uuid;
        }
        increaseAccountUsage(tmpLLMAccountDecrypt, ChatTextPlain);

        // 正常大部分场景不会进这里, 如果socket写入的字节比总字节还少，有可能还没写完，稍微延迟
        TimerEventLoop oneloop;
        oneloop.setTimeout(100);
        oneloop.exec();

        qCInfo(logWrapper) << "Sending chat text response - uuid:" << uuid;
        QMetaObject::invokeMethod(m_q, "chatTextReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QString, response.value("content").toString()));

        qCInfo(logWrapper) << "Request processing completed - uuid:" << uuid;
        return uuid;
    });

    QFutureWatcher<QString> *futureWatcher = new QFutureWatcher<QString>(this);
    QObject::connect(futureWatcher, &QFutureWatcher<QString>::finished, this, &SessionPrivate::onRequestTaskFinished);
    futureWatcher->setFuture(future);

    QStringList reply;
    reply << uuid;
    reply << "chatTextReceived";

    // 开启了stream才需要等待管道完全开启
    // 这里是多线程，可能已经quit了，还没来及exec
    if (stream && !isLoopQuit) {
        eventloop.exec();
    }

    return qMakePair(AIServer::NoError, reply);
}

QPair<AIServer::ErrorType, QStringList> SessionPrivate::requestInstFunction(LLMServerProxy tmpLLMAccount, const QString &conversation, const QJsonArray &funcs, const QVariantHash &params)
{
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);
    qCInfo(logWrapper) << "Starting instruction function request:" << uuid;
    
    if (!tmpLLMAccount.isValid()) {
        qCWarning(logWrapper) << "Invalid LLM account for instruction function request:" << uuid;
        QStringList reply;
        reply << uuid;
        reply << QCoreApplication::translate("SessionPrivate", "UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
        return qMakePair(AIServer::AccountInvalid, reply);
    }

    bool stream = params.value(PREDICT_PARAM_STREAM, false).toBool();
    qCDebug(logWrapper) << "Request" << uuid << "stream mode:" << stream;

    // 这里如果开启了管道，需要等管道完全开启才能返回
    QEventLoop eventloop;
    bool isLoopQuit = false;

    QFuture<QString> future = QtConcurrent::run(LLMThreadTaskMana::instance()->threadPool(tmpLLMAccount.id), [ =, &eventloop, &isLoopQuit]() {
        LLMServerProxy tmpLLMAccountLocal = tmpLLMAccount;

        // 免费账号需要解密
        LLMServerProxy tmpLLMAccountDecrypt = tmpLLMAccountLocal.decryptAccount();

        QSharedPointer<LLM> copilot = LLMVendor()->getCopilot(tmpLLMAccountDecrypt);
        if (copilot.isNull()) {
            qCWarning(logWrapper) << "Failed to get copilot instance - uuid:" << uuid;
            eventloop.quit();
            isLoopQuit = true;
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, AIServer::ContentAccessDenied), Q_ARG(QString, "Invalid LLM Account."));
            return uuid;
        }

        qCInfo(logWrapper) << "Initializing copilot - uuid:" << uuid 
                          << "model:" << LLMServerProxy::llmName(tmpLLMAccountDecrypt.model);
        copilot->loadParams(params);
        copilot->setCreatedId(uuid);
        copilot->switchStream(stream);
        qCDebug(logWrapper) << "Copilot initialized with stream mode:" << stream << " - uuid:" << uuid;

        connect(m_q, &Session::executionAborted, copilot.data(), &LLM::aborted);

        m_runTaskIds << uuid;
        LLMThreadTaskMana::instance()->addRequestTask(uuid, copilot.toWeakRef());

        if (tmpLLMAccountDecrypt.model == LLMChatModel::LOCAL_TEXT2IMAGE) {
            qCInfo(logWrapper) << "Handling local text2image model - uuid:" << uuid;
            eventloop.quit();
            isLoopQuit = true;
            return handleLocalModel(copilot, uuid, conversation, tmpLLMAccountDecrypt);
        }

        AppSocketServer socketServer(uuid, params.value(PREDICT_PARAM_NOJSONOUTPUT, false).toBool());
        qCDebug(logWrapper) << "Socket server created - uuid:" << uuid << socketServer.outputWithJson();

        // 开启流格式了，才会开启管道
        if (stream) {
            socketServer.startServer();
            connect(copilot.data(), &LLM::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);
            eventloop.quit();
            isLoopQuit = true;
            qCDebug(logWrapper) << "Stream mode initialized - uuid:" << uuid;
        }

        // 免费账号判断有效期和次数
        int errorType = -1;
        if (!checkLLMAccountStatus(uuid, tmpLLMAccountLocal, errorType)) {
            qCWarning(logWrapper) << "Account status check failed - uuid:" << uuid 
                                << "error type:" << errorType;
            return uuid;
        }
        qCDebug(logWrapper) << "Account status check passed - uuid:" << uuid;

        // 获取模型接口URL
        tmpLLMAccountDecrypt.url = tmpLLMAccountLocal.url;

        // 更新llm的账号
        copilot->updateAccount(tmpLLMAccountDecrypt);

        QJsonObject response;

        if (!handleAiServerRequest(copilot, uuid, response, conversation, funcs, tmpLLMAccountDecrypt)) {
            if (copilot->isReplied() && (copilot->lastError() == AIServer::OperationCanceledError))
                increaseAccountUsage(tmpLLMAccountDecrypt, ChatTextPlain);
            return uuid;
        }

        int chatAction = FunctionHandler::chatAction(response);
        increaseAccountUsage(tmpLLMAccountDecrypt, chatAction);
        qCDebug(logWrapper) << "Chat action determined - uuid:" << uuid 
                           << "action:" << chatAction;
        QMetaObject::invokeMethod(m_q, "chatAction", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, chatAction));

        if (chatAction != ChatFunctionCall) {
            qCWarning(logWrapper) << "Invalid function call action - uuid:" << uuid 
                                << "action:" << chatAction;
            int fcInvalidErr = AIServer::InstFunctionCallingInvalid;
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection,
                Q_ARG(QString, uuid),
                Q_ARG(int, fcInvalidErr),
                Q_ARG(QString, ServerCodeTranslation::serverCodeTranslation(fcInvalidErr, "").toUtf8())
                );
            return uuid;
        }

        qCInfo(logWrapper) << "Processing function call - uuid:" << uuid;
        QString directReply;
        const QJsonArray &functionCalls = FunctionHandler::instFuncCall(response, conversation, &directReply);
        if (directReply.isEmpty()) {
            qCDebug(logWrapper) << "No direct reply, sending function calls - uuid:" << uuid;
            QJsonDocument document(functionCalls);
            if (!handleAiServerRequest(copilot, uuid, response, document.toJson(QJsonDocument::Compact), funcs, tmpLLMAccountDecrypt))
                return uuid;
        } else {
            qCDebug(logWrapper) << "Using direct reply - uuid:" << uuid;
            response["content"] = directReply;
            copilot->textChainContent(directReply);
        }

        // 正常大部分场景不会进这里, 如果socket写入的字节比总字节还少，有可能还没写完，稍微延迟
        TimerEventLoop oneloop;
        oneloop.setTimeout(100);
        oneloop.exec();

        QMetaObject::invokeMethod(m_q, "chatTextReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QString, response.value("content").toString()));

        return uuid;
    });

    QFutureWatcher<QString> *futureWatcher = new QFutureWatcher<QString>(this);
    QObject::connect(futureWatcher, &QFutureWatcher<QString>::finished, this, &SessionPrivate::onRequestTaskFinished);
    futureWatcher->setFuture(future);

    QStringList reply;
    reply << uuid;
    reply << "chatTextReceived";

    // 开启了stream才需要等待管道完全开启
    // 这里是多线程，可能已经quit了，还没来及exec
    if (stream && !isLoopQuit) {
        eventloop.exec();
    }

    return qMakePair(AIServer::NoError, reply);
}

QPair<AIServer::ErrorType, QStringList> SessionPrivate::requestPlugin(LLMServerProxy tmpLLMAccount, const QString &conversation, const QVariantHash &params)
{
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);
    qCInfo(logWrapper) << "Starting plugin request:" << uuid;
    
    bool stream = params.value(PREDICT_PARAM_STREAM, false).toBool();
    qCDebug(logWrapper) << "Request" << uuid << "stream mode:" << stream;

    // 这里如果开启了管道，需要等管道完全开启才能返回
    QEventLoop eventloop;
    bool isLoopQuit = false;

    QFuture<QString> future = QtConcurrent::run(LLMThreadTaskMana::instance()->threadPool(tmpLLMAccount.id), [=, &eventloop, &isLoopQuit]() {
        QSharedPointer<LLM> copilot = LLMVendor()->getCopilot(tmpLLMAccount);
        if (copilot.isNull()) {
            eventloop.quit();
            isLoopQuit = true;

            TimerEventLoop oneloop;
            oneloop.setTimeout(100);
            oneloop.exec();

            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, AIServer::ContentAccessDenied), Q_ARG(QString, "Invalid LLM Account."));
            return uuid;
        }

        connect(m_q, &Session::executionAborted, copilot.data(), &LLM::aborted);
        copilot->loadParams(params);
        copilot->setCreatedId(uuid);
        copilot->switchStream(stream);
        qCDebug(logWrapper) << "Copilot initialized with stream mode:" << stream << " - uuid:" << uuid;

        // AppSocketServer 在本线程中需要一个eventloop处理事件。
        AppSocketServer socketServer(uuid, params.value(PREDICT_PARAM_NOJSONOUTPUT, false).toBool());
        qCDebug(logWrapper) << "Socket server created - uuid:" << uuid << socketServer.outputWithJson();

        // 开启流格式了，才会开启管道
        if (stream) {
            socketServer.startServer();
            connect(copilot.data(), &LLM::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);
            eventloop.quit();
            isLoopQuit = true;
            qCDebug(logWrapper) << "Stream mode initialized - uuid:" << uuid;
        }

        m_runTaskIds << uuid;
        LLMThreadTaskMana::instance()->addRequestTask(uuid, copilot.toWeakRef());

        QJsonArray functions;
        QString role = m_assistantProxy.id;
        qCDebug(logWrapper) << "Preparing request with role:" << role << " - uuid:" << uuid;

        QJsonObject response = copilot->predict(conversation, functions);

        // 如果结束了，只有一种可能，此对象被销毁了，销毁了就不要继续下面的流程了
        if (LLMThreadTaskMana::instance()->isFinished(uuid))
            return uuid;

        if (copilot->lastError() != AIServer::NoError) {
            qCWarning(logWrapper) << "Plugin chat error - uuid:" << uuid 
                                << "code:" << copilot->lastError() 
                                << "message:" << copilot->lastErrorString();
            handleRequestError(copilot, uuid);
            return uuid;
        }

        //ztb、xxzsk、yzsbs的引用数据
        if (response.contains("references") && (m_assistantProxy.id == "ztbbd" || m_assistantProxy.id == "xxzsk" || m_assistantProxy.id == "yzsbs") ) {
            qCInfo(logWrapper) << "Processing references for assistant:" << m_assistantProxy.id << " - uuid:" << uuid;
            QJsonObject root = response.value("references").toObject();
            QJsonDocument doc(root);
            QString datas = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
            QMetaObject::invokeMethod(m_q, "previewReference", Qt::QueuedConnection,
                                      Q_ARG(QString, datas));
            qCDebug(logWrapper) << "References processed and sent - uuid:" << uuid;
        }

        // AppSocketServer 正常大部分场景不会进这里, 如果socket写入的字节比总字节还少，有可能还没写完，稍微延迟
        TimerEventLoop oneloop;
        oneloop.setTimeout(100);
        oneloop.exec();

        QMetaObject::invokeMethod(m_q, "chatTextReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QString, response.value("content").toString()));

        return uuid;
    });

    QFutureWatcher<QString> *futureWatcher = new QFutureWatcher<QString>(this);
    QObject::connect(futureWatcher, &QFutureWatcher<QString>::finished, this, &SessionPrivate::onRequestTaskFinished);
    futureWatcher->setFuture(future);

    QStringList reply;
    reply << uuid;
    reply << "chatTextReceived";

    // 开启了stream才需要等待管道完全开启
    // 这里是多线程，可能已经quit了，还没来及exec
    if (stream && !isLoopQuit)
        eventloop.exec();

    return qMakePair(AIServer::NoError, reply);
}

QPair<AIServer::ErrorType, QStringList> SessionPrivate::requestAgent(LLMServerProxy tmpLLMAccount, const QString &conversation, const QVariantHash &params)
{
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);
    if (!tmpLLMAccount.isValid()) {
        QStringList reply;
        reply << uuid;
        reply << QCoreApplication::translate("SessionPrivate", "UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
        return qMakePair(AIServer::AccountInvalid, reply);
    }

    // 这里如果开启了管道，需要等管道完全开启才能返回
    QEventLoop eventloop;
    bool isLoopQuit = false;

    QFuture<QString> future = QtConcurrent::run(LLMThreadTaskMana::instance()->threadPool(tmpLLMAccount.id), [ =, &eventloop, &isLoopQuit]() {
        LLMServerProxy tmpLLMAccountLocal = tmpLLMAccount;

        // 免费账号需要解密
        LLMServerProxy tmpLLMAccountDecrypt = tmpLLMAccountLocal.decryptAccount();

        QSharedPointer<LLM> copilot = LLMVendor()->getCopilot(tmpLLMAccountDecrypt);
        QSharedPointer<LlmAgent> agent = AgentFactory::instance()->getAgent(params.value(PREDICT_PARAM_MCPAGENT).toString());
        if (copilot.isNull() || agent.isNull()) {
            eventloop.quit();
            isLoopQuit = true;

            TimerEventLoop oneloop;
            oneloop.setTimeout(100);
            oneloop.exec();

            QString msg = copilot.isNull() ? "Invalid LLM Account." : "Invalid Agent.";
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, AIServer::ContentAccessDenied), Q_ARG(QString, "Invalid LLM Account."));
            return uuid;
        }

        copilot->loadParams(params);
        copilot->setCreatedId(uuid);
        // must be stream
        copilot->switchStream(true);

        connect(m_q, &Session::executionAborted, copilot.data(), &LLM::aborted, Qt::DirectConnection);

        m_runTaskIds << uuid;
        LLMThreadTaskMana::instance()->addRequestTask(uuid, copilot.toWeakRef());

        AppSocketServer socketServer(uuid);

        // 开启流格式了，才会开启管道
        socketServer.startServer();
        connect(agent.data(), &LlmAgent::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);

        eventloop.quit();
        isLoopQuit = true;

        // 免费账号判断有效期和次数
        int errorType = -1;
        if (!checkLLMAccountStatus(uuid, tmpLLMAccountLocal, errorType)) {
            qWarning() << "Failed to check the validity period and usage times of the free account.";
            return uuid;
        }
        // 获取模型接口URL
        tmpLLMAccountDecrypt.url = tmpLLMAccountLocal.url;

        // 更新llm的账号
        copilot->updateAccount(tmpLLMAccountDecrypt);

        if (!agent->initialize()) {
            TimerEventLoop oneloop;
            oneloop.setTimeout(100);
            oneloop.exec();

            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, AIServer::ContentAccessDenied), Q_ARG(QString, tr("Agent server is not available.")));
            return uuid;
        }

        // 设置模型
        agent->setModel(copilot);
        
        QJsonObject response;
        {
            auto ary = QJsonDocument::fromJson(conversation.toUtf8()).array();
            auto question = ary.last();
            ary.pop_back();

            // 给点时间，等待流式输出socket建立连接
            {
                TimerEventLoop oneloop;
                oneloop.setTimeout(500);
                oneloop.exec();
            }

            response = agent->processRequest(question.toObject(), ary, params);

            // 如果结束了，只有一种可能，此对象被销毁了，销毁了就不要继续下面的流程了
            if (LLMThreadTaskMana::instance()->isFinished(uuid))
                return uuid;

            if (copilot->lastError() != AIServer::NoError) {
                // 给点时间，等待流式输出完成
                QThread::msleep(500);
                if (copilot->isReplied() && (copilot->lastError() == AIServer::OperationCanceledError))
                    increaseAccountUsage(tmpLLMAccountDecrypt, ChatTextPlain);

                qCWarning(logWrapper) << "Chat error - uuid:" << uuid
                                    << "code:" << copilot->lastError()
                                    << "message:" << copilot->lastErrorString();
                handleRequestError(copilot, uuid);
                return uuid;
            }
        }

        QMetaObject::invokeMethod(m_q, "chatAction", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, ChatTextPlain));
        increaseAccountUsage(tmpLLMAccountDecrypt, ChatTextPlain);

        // 正常大部分场景不会进这里, 如果socket写入的字节比总字节还少，有可能还没写完，稍微延迟
        TimerEventLoop oneloop;
        oneloop.setTimeout(100);
        oneloop.exec();

        QMetaObject::invokeMethod(m_q, "chatTextReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QString, response.value("content").toString()));

        return uuid;
    });

    QFutureWatcher<QString> *futureWatcher = new QFutureWatcher<QString>(this);
    QObject::connect(futureWatcher, &QFutureWatcher<QString>::finished, this, &SessionPrivate::onRequestTaskFinished);
    QObject::connect(futureWatcher, &QFutureWatcher<QString>::finished, this, [ = ]{
        McpConfigSyncer::instance()->fetchConfigFromServerAsync();
    });

    futureWatcher->setFuture(future);

    QStringList reply;
    reply << uuid;
    reply << "chatTextReceived";

    // 开启了stream才需要等待管道完全开启
    // 这里是多线程，可能已经quit了，还没来及exec
    if (!isLoopQuit) {
        eventloop.exec();
    }

    return qMakePair(AIServer::NoError, reply);
}

QString SessionPrivate::chatRequest(LLMServerProxy llmAccount, const QString &ctx, const QVariantHash &params)
{
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);
    qCInfo(logWrapper) << "Starting chat request:" << uuid;

    bool stream = params.value(PREDICT_PARAM_STREAM, false).toBool();
    bool increaseUse = params.value(PREDICT_PARAM_INCREASEUSE, false).toBool();
    qCDebug(logWrapper) << "Request" << uuid << "stream mode:" << stream << "increase use:" << increaseUse;

    // 这里如果开启了管道，需要等管道完全开启才能返回
    QEventLoop eventloop;
    bool isLoopQuit = false;

    QFuture<QString> future = QtConcurrent::run(LLMThreadTaskMana::instance()->threadPool(llmAccount.id), [=, &eventloop, &isLoopQuit]() {
        if (!llmAccount.isValid()) {
            qCWarning(logWrapper) << "Invalid LLM account for request:" << uuid;
            eventloop.quit();
            isLoopQuit = true;

            QString errInfo = QCoreApplication::translate("SessionPrivate", "UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
            onError(uuid, AIServer::AccountInvalid, errInfo);
            return uuid;
        }

        LLMServerProxy tmpLLMAccount = llmAccount;
        LLMServerProxy llmAccountDecrypt = tmpLLMAccount.decryptAccount();

        QSharedPointer<LLM> copilot = LLMVendor()->getCopilot(llmAccountDecrypt);
        if (copilot.isNull()) {
            qCWarning(logWrapper) << "Failed to get copilot instance - uuid:" << uuid;
            eventloop.quit();
            isLoopQuit = true;
            onError(uuid, AIServer::ContentAccessDenied, "Invalid LLM Account.");
            return uuid;
        }
        copilot->loadParams(params);
        copilot->setCreatedId(uuid);
        copilot->switchStream(stream);
        connect(m_q, &Session::executionAborted, copilot.data(), &LLM::aborted);

        m_runTaskIds << uuid;
        LLMThreadTaskMana::instance()->addRequestTask(uuid, copilot.toWeakRef());

        // AppSocketServer 在本线程中需要一个eventloop处理事件。
        AppSocketServer socketServer(uuid, params.value(PREDICT_PARAM_NOJSONOUTPUT, false).toBool());
        qCDebug(logWrapper) << "Socket server created - uuid:" << uuid << socketServer.outputWithJson();
        // 开启流格式了，才会开启管道
        if (stream) {
            socketServer.startServer();
            connect(copilot.data(), &LLM::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);
            eventloop.quit();
            isLoopQuit = true;
            qCDebug(logWrapper) << "Stream mode initialized - uuid:" << uuid;
        }

        // 免费账号判断有效期和次数
        int errorType = -1;
        if (!checkLLMAccountStatus(uuid, tmpLLMAccount, errorType)) {
            qCWarning(logWrapper) << "Account status check failed - uuid:" << uuid 
                                << "error type:" << errorType;
            return uuid;
        }
        // 获取模型接口URL
        llmAccountDecrypt.url = tmpLLMAccount.url;

        // 更新llm的账号
        copilot->updateAccount(llmAccountDecrypt);

        QJsonObject response = copilot->predict(ctx, QJsonArray());
        // 如果结束了，只有一种可能，此对象被销毁了，销毁了就不要继续下面的流程了
        if (LLMThreadTaskMana::instance()->isFinished(uuid))
            return uuid;

        if (copilot->lastError() != AIServer::NoError) {
            if (increaseUse && copilot->isReplied() && (copilot->lastError() == AIServer::OperationCanceledError))
                increaseAccountUsage(llmAccountDecrypt, ChatTextPlain);
            qCWarning(logWrapper) << "Chat error - uuid:" << uuid 
                                << "code:" << copilot->lastError() 
                                << "message:" << copilot->lastErrorString();
            handleRequestError(copilot, uuid);
            return uuid;
        }

        if (increaseUse)
            increaseAccountUsage(llmAccountDecrypt, ChatTextPlain);

        // AppSocketServer 正常大部分场景不会进这里, 如果socket写入的字节比总字节还少，有可能还没写完，稍微延迟
        TimerEventLoop oneloop;
        oneloop.setTimeout(100);
        oneloop.exec();

        QMetaObject::invokeMethod(m_q, "chatTextReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QString, response.value("content").toString()));
        return uuid;
    });

    QFutureWatcher<QString> *futureWatcher = new QFutureWatcher<QString>(this);
    QObject::connect(futureWatcher, &QFutureWatcher<QString>::finished, this, &SessionPrivate::onRequestTaskFinished);
    futureWatcher->setFuture(future);

    // 开启了stream才需要等待管道完全开启
    // 这里是多线程，可能已经quit了，还没来及exec
    if (stream && !isLoopQuit)
        eventloop.exec();

    return uuid;
}

QString SessionPrivate::genImageRequest(LLMServerProxy tmpLLMAccount, const QString &imageDesc)
{
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);
    qCInfo(logWrapper) << "Starting image generation request:" << uuid;

    if (!tmpLLMAccount.isValid()) {
        QString errInfo = QCoreApplication::translate("SessionPrivate", "UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
        onError(uuid, AIServer::AccountInvalid, errInfo);
        return uuid;
    }

    QFuture<QString> future = QtConcurrent::run(LLMThreadTaskMana::instance()->threadPool(tmpLLMAccount.id), [=]() {
        LLMServerProxy tmpLLMAccountLocal = tmpLLMAccount;

        // 免费账号需要解密
        LLMServerProxy tmpLLMAccountDecrypt = tmpLLMAccountLocal.decryptAccount();
        QSharedPointer<LLM> copilot = LLMVendor()->getCopilot(tmpLLMAccountDecrypt);
        copilot->setCreatedId(uuid);
        connect(m_q, &Session::executionAborted, copilot.data(), &LLM::aborted);

        m_runTaskIds << uuid;
        LLMThreadTaskMana::instance()->addRequestTask(uuid, copilot.toWeakRef());

        // 免费账号判断有效期和次数
        int errorType = -1;
        if (!checkLLMAccountStatus(uuid, tmpLLMAccountLocal, errorType)) {
            qCWarning(logWrapper) << "Account status check failed - uuid:" << uuid 
                                << "error type:" << errorType;
            return uuid;
        }
        // 获取模型接口URL
        tmpLLMAccountDecrypt.url = tmpLLMAccountLocal.url;

        // 更新llm的账号
        copilot->updateAccount(tmpLLMAccountDecrypt);

        QMetaObject::invokeMethod(m_q, "chatAction", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, ChatText2Image));

        if (errorType == AIServer::FREEACCOUNTTEXT2IMAGEUSAGELIMIT) {
            qCWarning(logWrapper) << "Text2image usage limit reached - uuid:" << uuid;
            QMetaObject::invokeMethod(m_q, "error",
                Q_ARG(QString, uuid),
                Q_ARG(int, errorType),
                Q_ARG(QString, ServerCodeTranslation::serverCodeTranslation(errorType, "").toUtf8())
                );
            return uuid;
        }

#ifdef TEXT_TO_IMAGE
        QList<QByteArray> imgData;
        if (LLMServerProxy::llmManufacturer(tmpLLMAccountDecrypt.model) == ModelManufacturer::BAIDU_WXQF ||
            LLMServerProxy::llmManufacturer(tmpLLMAccountDecrypt.model) == ModelManufacturer::XUNFEI_SPARKDESK) {
            // baidu、xunfei调用自己的文生图接口
            imgData = copilot->text2Image(imageDesc, 1);//文生图 1张
        } else {
            QString imagePrompt = GeneratePrompt::Translate2EnPrompt(imageDesc);
            QJsonObject tmpResponse;
            if (!handleAiServerRequest(copilot, uuid, tmpResponse, imagePrompt, QJsonArray(), qreal(0.9f), tmpLLMAccountDecrypt))
                return uuid;
            QString imageDescription = tmpResponse.value("content").toString();

            /**
             * @brief Request the image generate interface.
             * @param imageDescription: Image Description
             * @param n: Image Count.
             *
             * TODO: imageGenerate interface
             *
             * return ImagePath / Image Base64
             */

            QString imagePath = "";
            QImage image(imagePath);
            if (image.isNull()) {
                qWarning() << "Failed to load image!";
                // onError()
                return uuid;
            }

            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");
            QString base64Data = byteArray.toBase64();

            imgData.append(base64Data.toUtf8());
        }
#else
        QList<QByteArray> imgData = copilot->text2Image(imageDesc, 1);//文生图 1张
#endif

        if (copilot->lastError() != AIServer::NoError) {
            qCWarning(logWrapper) << "Image generation error - uuid:" << uuid 
                                << "code:" << copilot->lastError() 
                                << "message:" << copilot->lastErrorString();
            handleRequestError(copilot, uuid);
            return uuid;
        }

        QMetaObject::invokeMethod(m_q, "text2ImageReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QList<QByteArray>, imgData));

        increaseAccountUsage(tmpLLMAccountDecrypt, ChatText2Image);

        // 正常大部分场景不会进这里, 如果socket写入的字节比总字节还少，有可能还没写完，稍微延迟
        TimerEventLoop oneloop;
        oneloop.setTimeout(100);
        oneloop.exec();

        return uuid;
    });

    QFutureWatcher<QString> *futureWatcher = new QFutureWatcher<QString>(this);
    QObject::connect(futureWatcher, &QFutureWatcher<QString>::finished, this, &SessionPrivate::onRequestTaskFinished);
    futureWatcher->setFuture(future);

    return uuid;
}

QString SessionPrivate::searchRequest(LLMServerProxy llmAccount, const QString &ctx)
{
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);
    qCInfo(logWrapper) << "Starting search request:" << uuid;

    if (!llmAccount.isValid()) {
        QString errInfo = QCoreApplication::translate("SessionPrivate", "UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
        onError(uuid, AIServer::AccountInvalid, errInfo);
        return uuid;
    }

    if (!NetworkMonitor::getInstance().isOnline()) {
        qCWarning(logWrapper) << "Network offline, request aborted:" << uuid;
        QString errInfo = ServerCodeTranslation::serverCodeTranslation(AIServer::NetworkError, "").toUtf8();
        onError(uuid, AIServer::NetworkError, errInfo);
        return uuid;
    }

    LLMServerProxy tmpLLMAccount = llmAccount;
    LLMServerProxy llmAccountDecrypt = tmpLLMAccount.decryptAccount();

    QSharedPointer<LLM> copilot = LLMVendor()->getCopilot(llmAccountDecrypt);
    if (copilot.isNull()) {
        onError(uuid, AIServer::ContentAccessDenied, "Invalid LLM Account.");
        return uuid;
    }
    copilot->setCreatedId(uuid);
    connect(m_q, &Session::executionAborted, copilot.data(), &LLM::aborted);

    m_runTaskIds << uuid;
    LLMThreadTaskMana::instance()->addRequestTask(uuid, copilot.toWeakRef());

    // 免费账号判断有效期和次数
    int errorType = -1;
    if (!checkLLMAccountStatus(uuid, tmpLLMAccount, errorType)) {
        qCWarning(logWrapper) << "Failed to check free account validity period and usage times";
        return uuid;
    }
    // 获取模型接口URL
    llmAccountDecrypt.url = tmpLLMAccount.url;

    // 更新llm的账号
    copilot->updateAccount(llmAccountDecrypt);

    // 如果结束了，只有一种可能，此对象被销毁了，销毁了就不要继续下面的流程了
    if (LLMThreadTaskMana::instance()->isFinished(uuid))
        return uuid;

    QMetaObject::invokeMethod(m_q, "chatTextReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QString, Conversation::conversationLastUserData(ctx)));

    return uuid;
}

void SessionPrivate::claimUsageRequest(LLMServerProxy llmAccount)
{
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);
    qCInfo(logWrapper) << "Starting account limit request:" << uuid;

    if (!llmAccount.isValid())
        qCWarning(logWrapper) << "Invalid LLM account for request:" << uuid;

    QFuture<void> future = QtConcurrent::run(LLMThreadTaskMana::instance()->threadPool(llmAccount.id), [ = ]() {
        AIServer::ErrorType errorType;
        if (llmAccount.type != ModelType::FREE_NORMAL && llmAccount.type != ModelType::FREE_KOL) {
            qCWarning(logWrapper) << "Only free account can claim usage.";
            return;
        }

        int result = 0;
        QString resultMessage = "";
        int errorCode = UosFreeAccounts::instance().claimAccountUsage(llmAccount.account.apiKey, result, resultMessage);
        if (errorCode == QNetworkReply::NoError) {
            bool claimed = false;
            switch(result) {
            case 200: // 领取成功
                claimed = true;
                resultMessage = tr("Successfully Claimed");
                break;
            case 5001: // 账户不存在
                resultMessage = tr("Account not found");
                break;
            case 5002: // 仅支持DeepSeek账号领取额外额度
                resultMessage = tr("Only support deepseek account");
                break;
            case 5003: // 账号额度尚未用完 NOTE:仅老用户，在新手引导时存在此场景，无需告知用户该情况，默认告诉用户领取成功
                resultMessage = tr("Successfully Claimed");
                break;
            case 5004: // 已参与过活动，无法重复领取
                claimed = true;
                resultMessage = tr("You have already participated in the event and cannot claim the reward again.");
                break;
            case 500:  // 系统错误
                resultMessage = tr("Server system error");
                break;
            default:
                resultMessage = tr("Failed to Claim. Please Try Again.");
                break;
            }

            QMetaObject::invokeMethod(m_q, "sigClaimAccountUsageFinished", Qt::QueuedConnection,
                                      Q_ARG(bool, claimed),
                                      Q_ARG(QString, resultMessage)
                                      );
        } else {
            qCWarning(logWrapper) << "Network error checking account status:" << errorCode;
            QMetaObject::invokeMethod(m_q, "sigClaimAccountUsageFinished", Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(QString, tr("Failed to Claim. Please Try Again."))
                                      );
        }
    });
}

void SessionPrivate::checkUpdateAssistantAccount(const QString &assistantId)
{
    qCInfo(logWrapper) << "Checking and updating assistant account for ID:" << assistantId;
    
    QList<AssistantProxy> assistantLst = DbWrapper::localDbWrapper().queryAssistantList();
    assistantLst << LLMVendor()->queryAssistantList();

    const auto it = std::find_if(assistantLst.begin(), assistantLst.end(), [assistantId](const AssistantProxy & assistant) {
        return assistant.id == assistantId;
    });

    if (it != assistantLst.end()) {
        qCDebug(logWrapper) << "Found assistant:" << it->id << "name:" << it->displayName;
        m_assistantProxy = *it;
    } else {
        qCWarning(logWrapper) << "Assistant" << assistantId << "not found, using first available";
        m_assistantProxy = assistantLst.value(0);
    }
}

void SessionPrivate::onRequestTaskFinished()
{
    QFutureWatcher<QString> *watcher = dynamic_cast<QFutureWatcher<QString> *>(sender());
    if (watcher) {
        QString id = watcher->result();
        qCInfo(logWrapper) << "Request task completed:" << id;
        m_runTaskIds.removeAll(id);
        LLMThreadTaskMana::instance()->requestTaskFinished(id);
    }

    sender()->deleteLater();
}

void SessionPrivate::onError(const QString &uuid, int errCode, const QString &errInfo)
{
    qCWarning(logWrapper) << "Request error - uuid:" << uuid 
                         << "code:" << errCode 
                         << "message:" << errInfo;
                         
    QMetaObject::invokeMethod(m_q, "error",
                              Qt::QueuedConnection,
                              Q_ARG(QString, uuid),
                              Q_ARG(int, errCode),
                              Q_ARG(QString, errInfo));

}

QVariant SessionPrivate::getFAQ()
{
    qCInfo(logWrapper) << "Getting FAQ for assistant:" << m_assistantProxy.id;
    return LLMVendor()->getFAQ(m_assistantProxy.id);
}

void SessionPrivate::increaseAccountUsage(const LLMServerProxy &llmAccount, int chatAction) const
{
    if (llmAccount.type == ModelType::FREE_NORMAL || llmAccount.type == ModelType::FREE_KOL) {
        qCDebug(logWrapper) << "Increasing free account usage" << chatAction;
        UosFreeAccounts::instance().increaseUse(llmAccount.id.split("_")[0], chatAction);
    }
}

QString SessionPrivate::ragPromptBuild(const QString &conversation)
{
    QJsonArray ragConversion;
    QString ragQuery;
    if (!conversation.isEmpty()) {
        const QJsonDocument &document = QJsonDocument::fromJson(conversation.toUtf8());
        if (document.isArray()) {
            ragConversion = document.array();
            ragQuery = ragConversion.last().toObject().value("content").toString();
        } else {
            ragQuery = conversation;
        }
    }
    QElapsedTimer timer;
    timer.start();
    QString resultData = EmbeddingServer::getInstance().embeddingSearch(ragQuery, 10, m_assistantProxy.type);
    qint64 elapsed = timer.elapsed();
    if (resultData.isEmpty()) {
        QJsonObject reference;
        reference.insert("type", ExtentionType::KnowledgeBase);
        reference.insert("searchTime", elapsed / 1000.0);
        QString referensStr = QJsonDocument(reference).toJson(QJsonDocument::Compact);
        QMetaObject::invokeMethod(m_q, "previewReference", Qt::QueuedConnection, Q_ARG(QString, referensStr));

        return QString();
    }

    QJsonObject resultObj = QJsonDocument::fromJson(resultData.toUtf8()).object();
    QString knowleadge;
    QHash<QString, QVector<QString>> references;
    int index = 1;
    for (auto res : resultObj["result"].toArray()) {
        QString docPath = res.toObject().value("source").toString();
        QString docContent = res.toObject().value("content").toString();

        references[docPath].push_back(docContent);

        // 添加明确的片段标识符，帮助模型区分上下文
        QFileInfo docFile(docPath);
        knowleadge += QString("--- [Source %1: %2] ---\n").arg(QString::number(index), docFile.fileName());
        knowleadge += docContent;
        knowleadge += "\n\n";

        index++;
    }

    {
        // 引用文档
        QJsonObject reference;
        reference.insert("type", ExtentionType::KnowledgeBase);

        QJsonArray docArray;
        for (const QString &doc : references.keys()) {
            QJsonObject docObj;
            docObj.insert("docPath", doc);

            QJsonArray docContents;
            for (const QString &docContent : references.value(doc)) {
                docContents << docContent;
            }
            docObj.insert("docContents", docContents);

            docArray.append(docObj);
        }
        reference.insert("sources", docArray);
        reference.insert("searchTime", elapsed / 1000.0);

        QString referensStr = QJsonDocument(reference).toJson(QJsonDocument::Compact);
        QMetaObject::invokeMethod(m_q, "previewReference", Qt::QueuedConnection, Q_ARG(QString, referensStr));
    }

    QString ragPrompt;
    if (m_assistantProxy.type == AssistantType::UOS_SYSTEM_ASSISTANT || m_assistantProxy.type == AssistantType::DEEPIN_SYSTEM_ASSISTANT) {
        QString promptTemplate = QCoreApplication::translate("EAiPrompt", "---Role---\n" \
                                                      "You are a helpful assistant, answering questions about the background knowledge provided.\n\n" \
                                                      "---Goal---\n" \
                                                      "Respond to users' questions, incorporating any relevant common sense.If you don't know the answer, just say so. Don't make it up.\n\n" \
                                                      "---knowledge---\n" \
                                                      "%1\n\n" \
                                                      "---question---\n" \
                                                      "%2\n");
        ragPrompt = promptTemplate.arg(knowleadge).arg(ragQuery);
    } else {
        if (references.isEmpty()) {
            QString promptTemplate = QCoreApplication::translate("EAiPrompt", "The output answer starts with \"No relevant information was found in your knowledge base.\"" \
                                                          "---question--\n" \
                                                          ":%1\n");
            ragPrompt = promptTemplate.arg(ragQuery);
        } else {
            QString promptTemplate = QCoreApplication::translate("EAiPrompt", "---Role---\n" \
                                                          "You are a helpful assistant, answering questions about the background knowledge provided.\n\n" \
                                                          "---Goal---\n" \
                                                          "Respond to users' questions, incorporating any relevant common sense.If you don't know the answer, just say so. Don't make it up.\n\n" \
                                                          "---knowledge---\n" \
                                                          "%1\n\n" \
                                                          "---question---\n" \
                                                          "%2");
            ragPrompt = promptTemplate.arg(knowleadge).arg(ragQuery);
        }
    }

    ragConversion.replace(ragConversion.size() - 1, QJsonObject({ { "role", "user" }, {"content", ragPrompt} }));
    return QJsonDocument(ragConversion).toJson();
}
