#include "session_p.h"
#include "session.h"
#include "timereventloop.h"
#include "appsocketserver.h"
#include "llmutils.h"
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

SessionPrivate::SessionPrivate(Session *session, const QString &appId)
    : m_appId(appId)
    , m_q(session)
{
    QString llmId = DbWrapper::localDbWrapper().queryCurLlmIdByAppId(appId);
    checkUpdateLLmAccount(llmId);

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
        errorString = QCoreApplication::translate("SessionPrivate", "Connection failed, please check the network.");;
    } else {
        errorCode = copilot->lastError();
        errorString = copilot->lastErrorString();
    }

    QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, errorCode), Q_ARG(QString, errorString));
    return true;
}

bool SessionPrivate::handleAiServerRequest(QSharedPointer<LLM> copilot, const QString &uuid
                                           , QJsonObject &response, const QString &conversation
                                           , const QJsonArray &functions, qreal temperature, LLMChatModel model, const QString &userName)
{
#ifdef DEBUG_LOG
    qWarning() << "User Name = " << userName << "Chat Model = " << LLMServerProxy::llmName(model) << "ChatSeesion Request = " << conversation;
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
            UosSimpleLog::instance().addLog({UosLogType::FailedRetry, m_appId, "", QDateTime::currentDateTime(), LLMServerProxy::llmName(model), "", 0});
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
    const QList<LLMServerProxy> &accountLst = DbWrapper::localDbWrapper().queryLlmList();
    if (accountLst.isEmpty()) {
        return LLMServerProxy();
    }

    //如果接口设置模型了，用接口的，否则用默认的
    LLMServerProxy tmpLLMAccount;
    const auto it = std::find_if(accountLst.begin(), accountLst.end(), [llmId](const LLMServerProxy & account) {
        return account.id == llmId;
    });

    if (it != accountLst.end()) {
        tmpLLMAccount = *it;
    } else {
        tmpLLMAccount = m_appServerProxy;
    }

    return tmpLLMAccount;
}

bool SessionPrivate::checkLLMAccountStatus(const QString &uuid, const LLMServerProxy &llmAccount)
{
    // 免费账号判断有效期和次数
    if (llmAccount.type == ModelType::FREE_NORMAL || llmAccount.type == ModelType::FREE_KOL) {
        int available = 0;
        int errorCode = UosFreeAccounts::instance().getDeterAccountLegal(llmAccount.account.apiKey, available);
        int errorType = -1;

        switch (errorCode) {
        case QNetworkReply::NoError:
            if (available == 1) {
                errorType = AIServer::FREEACCOUNTEXPIRED;
            } else if (available == 2) {
                errorType = AIServer::FREEACCOUNTUSAGELIMIT;
            }
            break;
        default:
            errorType = AIServer::NetworkError;
        }

        if (errorType != -1) {
            QMetaObject::invokeMethod(m_q, "error", Qt::QueuedConnection,
                                      Q_ARG(QString, uuid),
                                      Q_ARG(int, errorType),
                                      Q_ARG(QString, ServerCodeTranslation::serverCodeTranslation(errorType, "").toUtf8())
                                     );
            return false;
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
                                         , QDateTime::currentDateTime(), LLMServerProxy::llmName(llmAccount.model)
                                         , QString::number(llmAccount.type), copilot->lastError() == AIServer::NoError});
    }

    if (copilot->lastError() != AIServer::NoError) {
        handleRequestError(copilot, uuid);
        return uuid;
    }

    QMetaObject::invokeMethod(m_q, "text2ImageReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QList<QByteArray>, imgData));
    return uuid;
}

QPair<AIServer::ErrorType, QStringList> SessionPrivate::requestChatFunctionText(const QString &llmId, const QString &conversation, bool stream, qreal temperature)
{
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);

    LLMServerProxy tmpLLMAccount = queryValidServerAccount(llmId);
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
        // 用户日志收集
        if (DbWrapper::localDbWrapper().getUserExpState() == 1) {
            UosSimpleLog::instance().addLog({UosLogType::UserInput, m_appId, conversation, QDateTime::currentDateTime(), LLMServerProxy::llmName(tmpLLMAccount.model), QString::number(tmpLLMAccount.type), 0});
        }

        // 免费账号需要解密
        LLMServerProxy tmpLLMAccountDecrypt = tmpLLMAccount.decryptAccount();
        QSharedPointer<LLM> copilot = LLMUtils::getCopilot(tmpLLMAccountDecrypt);
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
        if (!checkLLMAccountStatus(uuid, tmpLLMAccount)) {
            qWarning() << "Failed to check the validity period and usage times of the free account.";
            return uuid;
        }

        QJsonObject appFunctions;
        // functions 目前只开放给助手自己使用
        if (qApp->applicationName()  == m_appId)
            appFunctions = FunctionHandler::queryAppFunctions(m_needQueryFunctions);

        QJsonObject response;
        QJsonArray functions = FunctionHandler::functions(appFunctions);

        if (!handleAiServerRequest(copilot, uuid, response, conversation, functions, temperature, tmpLLMAccountDecrypt.model, tmpLLMAccountDecrypt.name))
            return uuid;

        if (tmpLLMAccountDecrypt.type == ModelType::FREE_NORMAL || tmpLLMAccountDecrypt.type == ModelType::FREE_KOL) {
            UosFreeAccounts::instance().increaseUse(tmpLLMAccountDecrypt.id.split("_")[0]);
        }

        int chatAction = FunctionHandler::chatAction(response);
        QMetaObject::invokeMethod(m_q, "chatAction", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(int, chatAction));

        if (chatAction & ChatText2Image) {
            QJsonObject tmpResponse;
            disconnect(copilot.data(), &LLM::readyReadChatDeltaContent, &socketServer, &AppSocketServer::sendDataToClient);

            QString prompt = Text2ImageHandler::imagePrompt(response, conversation);
            if (!handleAiServerRequest(copilot, uuid, tmpResponse, prompt, QJsonArray(), temperature, tmpLLMAccountDecrypt.model, tmpLLMAccountDecrypt.name))
                return uuid;

            QList<QByteArray> imgData = copilot->text2Image(tmpResponse.value("content").toString(), 4);
            if (DbWrapper::localDbWrapper().getUserExpState() == 1) {
                UosSimpleLog::instance().addLog({UosLogType::TextToImageResult, m_appId, Conversation::conversationLastUserData(conversation)
                                                 , QDateTime::currentDateTime(), LLMServerProxy::llmName(tmpLLMAccount.model)
                                                 , QString::number(tmpLLMAccount.type), copilot->lastError() == AIServer::NoError});
            }

            if (copilot->lastError() != AIServer::NoError) {
                handleRequestError(copilot, uuid);
                return uuid;
            }

            QMetaObject::invokeMethod(m_q, "text2ImageReceived", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QList<QByteArray>, imgData));
            return uuid;
        }

        if (chatAction & ChatAction::ChatFunctionCall) {
            const QJsonArray &functionCalls = FunctionHandler::functionCall(response, conversation);

            QJsonDocument document(functionCalls);
            if (!handleAiServerRequest(copilot, uuid, response, document.toJson(QJsonDocument::Compact), functions, temperature, tmpLLMAccountDecrypt.model, tmpLLMAccountDecrypt.name))
                return uuid;
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

void SessionPrivate::checkUpdateLLmAccount(const QString &llmId)
{
    const QList<LLMServerProxy> &accounLst = DbWrapper::localDbWrapper().queryLlmList();
    const auto it = std::find_if(accounLst.begin(), accounLst.end(), [llmId](const LLMServerProxy & account) {
        return account.id == llmId;
    });

    if (it != accounLst.end()) {
        m_appServerProxy = *it;
    } else {
        m_appServerProxy = accounLst.value(0);
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
