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

#include <QtConcurrent>
#include <QFileSystemWatcher>

using namespace uos_ai;

SessionPrivate::SessionPrivate(Session *session, const QString &appId)
    : m_appId(appId)
    , m_q(session)
{
#ifdef ENABLE_MODEL_PLUGIN
    m_llmVendor.initModelPlugin();
#endif

    {
        QString assistantId = DbWrapper::localDbWrapper().queryCurAssistantIdByAppId(appId);
        checkUpdateAssistantAccount(assistantId);
    }

    {
        QString llmId = DbWrapper::localDbWrapper().queryLlmIdByAssistantId(m_assistantProxy.id);
        m_appServerProxy = m_llmVendor.queryValidServerAccount(llmId);
        if (!m_appServerProxy.isValid()) {
            auto supLLM = m_llmVendor.queryServerAccountByRole(m_assistantProxy);
            if (!supLLM.isEmpty())
                m_appServerProxy = supLLM.first();
            else
                qWarning() << "no llm for" << m_assistantProxy.id << m_assistantProxy.displayName;
        }
    }

    QFileSystemWatcher *watcher = new QFileSystemWatcher(this);
    watcher->addPath(FunctionHandler::functionPluginPath());

    connect(watcher, &QFileSystemWatcher::directoryChanged, this, [ = ](const QString & path) {
        m_needQueryFunctions = true;
    });

    connect(watcher, &QFileSystemWatcher::fileChanged, this, [ = ](const QString & path) {
        m_needQueryFunctions = true;
    });
}

SessionPrivate::~SessionPrivate()
{
    for (const QString &id : m_runTaskIds) {
        LLMThreadTaskMana::instance()->requestTaskFinished(id);
    }
}

void SessionPrivate::cancelRequestTask(const QString &id)
{
    m_runTaskIds.removeAll(id);
    LLMThreadTaskMana::instance()->cancelRequestTask(id);
}

bool SessionPrivate::handleRequestError(QSharedPointer<LLM> copilot, const QString &uuid)
{
    int errorCode;
    QString errorString;

    if (!NetworkMonitor::getInstance().isOnline()) {
        errorCode = AIServer::NetworkError;
        errorString = ServerCodeTranslation::serverCodeTranslation(errorCode, "");
    } else {
        errorCode = copilot->lastError();
        errorString = copilot->lastErrorString();
    }

    QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, errorCode), Q_ARG(QString, errorString));
    return true;
}

bool SessionPrivate::handleAiServerRequest(QSharedPointer<LLM> copilot, const QString &uuid
                                           , QJsonObject &response, const QString &conversation
                                           , const QJsonArray &functions, qreal temperature, const LLMServerProxy &llmAccount)
{
#ifdef DEBUG_LOG
    qWarning() << "User Name = " << llmAccount.name << "Chat Model = " << LLMServerProxy::llmName(llmAccount.model) << "ChatSeesion Request = " << conversation;
#endif

    QString maybeResponse = ChatFixedResponse::checkRequestText(conversation);
    if (!maybeResponse.isEmpty()) {
        response["content"] = maybeResponse;

        // 给点延时，直接返回，接收端可能还没来得及连接
        TimerEventLoop loop;
        loop.setTimeout(20);
        loop.exec();

        emit copilot->readyReadChatDeltaContent(maybeResponse);
        return true;
    }

    for (int i = 0; i < 3; i++) {
        response = copilot->predict(conversation, functions, QString(), temperature);

        // 速率被限制了，尝试请求下
        if (copilot->lastError() != AIServer::ServerRateLimitError && copilot->lastError() != AIServer::TimeoutError) {
            break;
        }

        int timeout = 500;
        if (copilot->lastError() == AIServer::ServerRateLimitError) {
            // 失败重试记录
            timeout = 1000;
            UosSimpleLog::instance().addLog({UosLogType::FailedRetry, m_appId, "",
                                             QDateTime::currentDateTime(), LLMServerProxy::llmName(llmAccount.model, !llmAccount.url.isEmpty()), "", "", 0});
        }

        TimerEventLoop loop;
        connect(copilot.data(), &LLM::aborted, &loop, &TimerEventLoop::quit);
        loop.setTimeout(timeout);
        loop.exec();
        disconnect(copilot.data(), &LLM::aborted, &loop, &TimerEventLoop::quit);
    }

#ifdef DEBUG_LOG
    qWarning() << "ChatSeesion Response = " << response;
#endif

    // 如果结束了，只有一种可能，此对象被销毁了，销毁了就不要继续下面的流程了
    if (LLMThreadTaskMana::instance()->isFinished(uuid)) {
        return false;
    }

    if (copilot->lastError() != AIServer::NoError) {
        qWarning() << "ChatSeesion Error = " << copilot->lastError() << copilot->lastErrorString();
        handleRequestError(copilot, uuid);
        return false;
    }

    return true;
}

LLMServerProxy SessionPrivate::queryValidServerAccount(const QString &llmId)
{
    LLMServerProxy tmpLLMAccount = m_llmVendor.queryValidServerAccount(llmId);
    if (!tmpLLMAccount.isValid())
        tmpLLMAccount = m_appServerProxy;

    return tmpLLMAccount;
}

bool SessionPrivate::checkLLMAccountStatus(const QString &uuid, LLMServerProxy &llmAccount, int &errorType)
{
    // 免费账号判断有效期和次数
    if (llmAccount.type == ModelType::FREE_NORMAL || llmAccount.type == ModelType::FREE_KOL) {
        int available = 0;
        QString modelUrl;
        int errorCode = UosFreeAccounts::instance().getDeterAccountLegal(llmAccount.account.apiKey, available, modelUrl);
        if (!modelUrl.isEmpty())
            llmAccount.url = modelUrl;
        auto serverCodeTranslation = [=](int errorType) {
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection,
                Q_ARG(QString, uuid),
                Q_ARG(int, errorType),
                Q_ARG(QString, ServerCodeTranslation::serverCodeTranslation(errorType, "").toUtf8())
                );
        };

        if (errorCode != QNetworkReply::NoError) {
            errorType = AIServer::NetworkError;
            serverCodeTranslation(errorType);
            return false;
        }

        switch (available) {
        case 1:
            errorType = AIServer::FREEACCOUNTEXPIRED;
            serverCodeTranslation(errorType);
            return false;
        case 2:
            errorType = AIServer::FREEACCOUNTUSAGELIMIT;
            serverCodeTranslation(errorType);
            return false;
        case 5:
            errorType = AIServer::FREEACCOUNTCHATUSAGELIMIT;
            break;
        case 6:
            errorType = AIServer::FREEACCOUNTTEXT2IMAGEUSAGELIMIT;
            break;
        default:
            break;
        }
    }
    return true;
}

QString SessionPrivate::handleLocalModel(QSharedPointer<LLM> copilot, const QString &uuid, const QString &conversation, const LLMServerProxy &llmAccount)
{
    QString prompt = Conversation::conversationLastUserData(conversation);

    QList<QByteArray> imgData = copilot->text2Image(prompt, 4);
    QMetaObject::invokeMethod(m_q, "chatAction", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, ChatText2Image));
    if (DbWrapper::localDbWrapper().getUserExpState() == 1) {
        UosSimpleLog::instance().addLog({UosLogType::TextToImageResult, m_appId, Conversation::conversationLastUserData(conversation)
                                         , QDateTime::currentDateTime(), LLMServerProxy::llmName(llmAccount.model, !llmAccount.url.isEmpty())
                                         , QString::number(llmAccount.type), AssistantProxy::assistantName(m_q->currentAssistantType()), copilot->lastError() == AIServer::NoError});
    }

    if (copilot->lastError() != AIServer::NoError) {
        handleRequestError(copilot, uuid);
        return uuid;
    }

    QMetaObject::invokeMethod(m_q, "text2ImageReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QList<QByteArray>, imgData));
    return uuid;
}

QPair<AIServer::ErrorType, QStringList> SessionPrivate::requestChatFunctionText(LLMServerProxy tmpLLMAccount, const QString &conversation, bool stream, qreal temperature, bool isFAQGeneration)
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

        // 用户日志收集
        if (DbWrapper::localDbWrapper().getUserExpState() == 1) {
            UosSimpleLog::instance().addLog(
            {UosLogType::UserInput, m_appId, conversation, QDateTime::currentDateTime(),
             LLMServerProxy::llmName(tmpLLMAccount.model, !tmpLLMAccount.url.isEmpty()), QString::number(tmpLLMAccount.type),
             AssistantProxy::assistantName(m_q->currentAssistantType()), 0});
        }

        // 免费账号需要解密
        LLMServerProxy tmpLLMAccountDecrypt = tmpLLMAccountLocal.decryptAccount();

        QSharedPointer<LLM> copilot = m_llmVendor.getCopilot(tmpLLMAccountDecrypt);
        if (copilot.isNull()) {
            eventloop.quit();
            isLoopQuit = true;
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, AIServer::ContentAccessDenied), Q_ARG(QString, "Invalid LLM Account."));
            return uuid;
        }

        copilot->setCreatedId(uuid);
        copilot->switchStream(stream);

        connect(m_q, &Session::executionAborted, copilot.data(), &LLM::aborted);

        m_runTaskIds << uuid;
        LLMThreadTaskMana::instance()->addRequestTask(uuid, copilot.toWeakRef());

        if (tmpLLMAccountDecrypt.model == LLMChatModel::LOCAL_TEXT2IMAGE) {
            eventloop.quit();
            isLoopQuit = true;
            return handleLocalModel(copilot, uuid, conversation, tmpLLMAccountDecrypt);
        }

        AppSocketServer socketServer(uuid);

        // 开启流格式了，才会开启管道
        if (stream) {
            socketServer.startServer();
            connect(copilot.data(), &LLM::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);
            eventloop.quit();
            isLoopQuit = true;
        }

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

        QJsonObject appFunctions;
        // functions 目前只开放给助手自己使用
        if (qApp->applicationName()  == m_appId)
            appFunctions = FunctionHandler::queryAppFunctions(m_needQueryFunctions);

        QJsonObject response;
        QJsonArray functions = FunctionHandler::functions(appFunctions);

        if (errorType == AIServer::FREEACCOUNTCHATUSAGELIMIT)
            disconnect(copilot.data(), &LLM::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);

        if (!handleAiServerRequest(copilot, uuid, response, conversation, functions, temperature, tmpLLMAccountDecrypt))
            return uuid;

        int chatAction = FunctionHandler::chatAction(response);
        QMetaObject::invokeMethod(m_q, "chatAction", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, chatAction));

        if (chatAction & ChatText2Image) {
            if (errorType == AIServer::FREEACCOUNTTEXT2IMAGEUSAGELIMIT) {
                QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection,
                    Q_ARG(QString, uuid),
                    Q_ARG(int, errorType),
                    Q_ARG(QString, ServerCodeTranslation::serverCodeTranslation(errorType, "").toUtf8())
                    );
                return uuid;
            }

            QJsonObject tmpResponse;
            disconnect(copilot.data(), &LLM::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);

            QString prompt = Text2ImageHandler::imagePrompt(response, conversation);
            if (!handleAiServerRequest(copilot, uuid, tmpResponse, prompt, QJsonArray(), temperature, tmpLLMAccountDecrypt))
                return uuid;

            QList<QByteArray> imgData = copilot->text2Image(tmpResponse.value("content").toString(), 1);//文生图 1张
            if (DbWrapper::localDbWrapper().getUserExpState() == 1) {
                UosSimpleLog::instance().addLog({UosLogType::TextToImageResult, m_appId, Conversation::conversationLastUserData(conversation)
                                                 , QDateTime::currentDateTime(), LLMServerProxy::llmName(tmpLLMAccount.model, !tmpLLMAccount.url.isEmpty())
                                                 , QString::number(tmpLLMAccount.type), AssistantProxy::assistantName(m_q->currentAssistantType()), copilot->lastError() == AIServer::NoError});
            }

            if (copilot->lastError() != AIServer::NoError) {
                handleRequestError(copilot, uuid);
                return uuid;
            }

            QMetaObject::invokeMethod(m_q, "text2ImageReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QList<QByteArray>, imgData));

            if (!isFAQGeneration && (tmpLLMAccountDecrypt.type == ModelType::FREE_NORMAL || tmpLLMAccountDecrypt.type == ModelType::FREE_KOL)) {
                UosFreeAccounts::instance().increaseUse(tmpLLMAccountDecrypt.id.split("_")[0], chatAction);
            }
            return uuid;
        }

        if (errorType == AIServer::FREEACCOUNTCHATUSAGELIMIT) {
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection,
                Q_ARG(QString, uuid),
                Q_ARG(int, errorType),
                Q_ARG(QString, ServerCodeTranslation::serverCodeTranslation(errorType, "").toUtf8())
                );
            return uuid;
        }

        if (!isFAQGeneration && (tmpLLMAccountDecrypt.type == ModelType::FREE_NORMAL || tmpLLMAccountDecrypt.type == ModelType::FREE_KOL)) {
            UosFreeAccounts::instance().increaseUse(tmpLLMAccountDecrypt.id.split("_")[0], chatAction);
        }

        if (chatAction & ChatAction::ChatFunctionCall) {
            QString directReply;
            const QJsonArray &functionCalls = FunctionHandler::functionCall(response, conversation, &directReply);
            if (directReply.isEmpty()) {
                QJsonDocument document(functionCalls);
                if (!handleAiServerRequest(copilot, uuid, response, document.toJson(QJsonDocument::Compact), functions, temperature, tmpLLMAccountDecrypt))
                    return uuid;
            } else {
                response["content"] = directReply;
                emit copilot->readyReadChatDeltaContent(directReply);
            }
        }

        // 正常大部分场景不会进这里, 如果socket写入的字节比总字节还少，有可能还没写完，稍微延迟
        TimerEventLoop oneloop;
        oneloop.setTimeout(100);
        oneloop.exec();

        QMetaObject::invokeMethod(m_q, "chatTextReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QString, response.value("content").toString()));

        return uuid;
    });

    QFutureWatcher<void> *futureWatcher = new QFutureWatcher<void>(this);
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

QPair<AIServer::ErrorType, QStringList> SessionPrivate::requestPlugin(LLMServerProxy tmpLLMAccount, const QString &conversation, bool stream, qreal temperature)
{
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);

    // 这里如果开启了管道，需要等管道完全开启才能返回
    QEventLoop eventloop;
    bool isLoopQuit = false;

    QFuture<QString> future = QtConcurrent::run(LLMThreadTaskMana::instance()->threadPool(tmpLLMAccount.id), [=, &eventloop, &isLoopQuit]() {
        QSharedPointer<LLM> copilot = m_llmVendor.getCopilot(tmpLLMAccount);
        if (copilot.isNull()) {
            eventloop.quit();
            isLoopQuit = true;
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, AIServer::ContentAccessDenied), Q_ARG(QString, "Invalid LLM Account."));
            return uuid;
        }

        connect(m_q, &Session::executionAborted, copilot.data(), &LLM::aborted);
        copilot->setCreatedId(uuid);
        copilot->switchStream(stream);

        // AppSocketServer 在本线程中需要一个eventloop处理事件。
        AppSocketServer socketServer(uuid);

        // 开启流格式了，才会开启管道
        if (stream) {
            socketServer.startServer();
            connect(copilot.data(), &LLM::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);
            eventloop.quit();
            isLoopQuit = true;
        }

        m_runTaskIds << uuid;
        LLMThreadTaskMana::instance()->addRequestTask(uuid, copilot.toWeakRef());

        QJsonArray functions;
        QString role = m_assistantProxy.id;

        QJsonObject response = copilot->predict(conversation, functions, role, temperature);

        // 如果结束了，只有一种可能，此对象被销毁了，销毁了就不要继续下面的流程了
        if (LLMThreadTaskMana::instance()->isFinished(uuid))
            return uuid;

        if (copilot->lastError() != AIServer::NoError) {
            qWarning() << "plugin chat error" << copilot->lastError() << copilot->lastErrorString();
            handleRequestError(copilot, uuid);
            return uuid;
        }

        // AppSocketServer 正常大部分场景不会进这里, 如果socket写入的字节比总字节还少，有可能还没写完，稍微延迟
        TimerEventLoop oneloop;
        oneloop.setTimeout(100);
        oneloop.exec();

        QMetaObject::invokeMethod(m_q, "chatTextReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QString, response.value("content").toString()));

        return uuid;
    });

    QFutureWatcher<void> *futureWatcher = new QFutureWatcher<void>(this);
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

void SessionPrivate::checkUpdateAssistantAccount(const QString &assistantId)
{
    QList<AssistantProxy> assistantLst = m_llmVendor.queryAssistantList();
    assistantLst << DbWrapper::localDbWrapper().queryAssistantList();

    const auto it = std::find_if(assistantLst.begin(), assistantLst.end(), [assistantId](const AssistantProxy & assistant) {
        return assistant.id == assistantId;
    });

    if (it != assistantLst.end()) {
        m_assistantProxy = *it;
    } else {
        m_assistantProxy = assistantLst.value(0);
    }
}

void SessionPrivate::onRequestTaskFinished()
{
    QFutureWatcher<QString> *watcher = dynamic_cast<QFutureWatcher<QString> *>(sender());
    if (watcher) {
        QString id = watcher->result();
        m_runTaskIds.removeAll(id);
        LLMThreadTaskMana::instance()->requestTaskFinished(id);
    }

    sender()->deleteLater();
}

QVariant SessionPrivate::getFAQ()
{
    return m_llmVendor.getFAQ(m_assistantProxy.id);
}
