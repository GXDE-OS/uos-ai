#include "appdbusobject.h"
#include "session.h"
#include "llmutils.h"

#include <QtDBus>

AppDbusObject::AppDbusObject(const QString &appId)
    : QObject(nullptr)
{
    m_chatSession = new Session(appId);
    connect(m_chatSession, &Session::error, this, &AppDbusObject::error, Qt::QueuedConnection);
    connect(m_chatSession, &Session::chatTextReceived, this, &AppDbusObject::chatTextReceived, Qt::QueuedConnection);
    connect(m_chatSession, &Session::llmAccountLstChanged, this, [this](const QString & currentAccountId, const QString & accountLst) {
        emit llmAccountLstChanged(currentAccountId, queryLLMAccountList());
    }, Qt::QueuedConnection);
}

AppDbusObject::~AppDbusObject()
{
    m_chatSession->deleteLater();
    m_chatSession = nullptr;
}

void AppDbusObject::cancelRequestTask(const QString &id)
{
    m_chatSession->cancelRequestTask(id);
}

void AppDbusObject::updateLLMAccount()
{
    m_chatSession->updateLLMAccount();
}

void AppDbusObject::executionAborted()
{
    emit m_chatSession->executionAborted();
}

QStringList AppDbusObject::requestChatText(const QString &llmId, const QString &conversation, qreal temperature, bool stream)
{
    uint pid = QDBusConnection::sessionBus().interface()->servicePid(message().service());
    if (m_chatSession->appId() != LLMUtils::queryAppId(pid)) {
        qWarning() << LLMUtils::queryAppId(pid) << " no permission to access!";
        sendErrorReply(QDBusError::AccessDenied, "no permission to access!");
        return QStringList();
    }

    const QPair<AIServer::ErrorType, QStringList> &chatText = m_chatSession->requestChatText(llmId, conversation, temperature, stream);
    if (chatText.first != AIServer::NoError) {
        sendErrorReply(QDBusError::NoServer, chatText.second.value(1));
        return QStringList();
    }

    return chatText.second;
}

bool AppDbusObject::setCurrentLLMAccountId(const QString &id)
{
    return m_chatSession->setCurrentLLMAccountId(id);
}

QString AppDbusObject::currentLLMAccountId()
{
    return m_chatSession->currentLLMAccountId();
}

QString AppDbusObject::queryLLMAccountList()
{
    QList<LLMChatModel> excludes;
    excludes << LLMChatModel::LOCAL_TEXT2IMAGE;
    return m_chatSession->queryLLMAccountList(excludes);
}

void AppDbusObject::launchLLMUiPage(bool showAddllmPage)
{
    emit launchUI(showAddllmPage);
}
