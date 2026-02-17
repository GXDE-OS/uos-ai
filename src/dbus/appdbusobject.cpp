#include "appdbusobject.h"
#include "session.h"
#include "llmutils.h"
#include "private/welcomedialog.h"
#include "dbwrapper.h"

#include <QtDBus>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

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
    if (!checkAgreement())
        return;

    qCDebug(logDBus) << "Cancelling request task with id:" << id;
    m_chatSession->cancelRequestTask(id);
}

void AppDbusObject::updateLLMAccount()
{
    if (!checkAgreement())
        return;

    qCInfo(logDBus) << "Updating LLM account";
    m_chatSession->updateLLMAccount();
}

void AppDbusObject::executionAborted()
{
    if (!checkAgreement())
        return;

    qCWarning(logDBus) << "Execution aborted";
    emit m_chatSession->executionAborted();
}

QStringList AppDbusObject::requestChatText(const QString &llmId, const QString &conversation, qreal temperature, bool stream)
{
    if (!checkAgreement())
        return QStringList();

    uint pid = QDBusConnection::sessionBus().interface()->servicePid(message().service());
    if (m_chatSession->appId() != LLMUtils::queryAppId(pid)) {
        qCWarning(logDBus) << "Access denied for appId:" << LLMUtils::queryAppId(pid);
        sendErrorReply(QDBusError::AccessDenied, "no permission to access!");
        return QStringList();
    }

    qCDebug(logDBus) << "Requesting chat text with llmId:" << llmId << "temperature:" << temperature << "stream:" << stream;
    QVariantHash params = {
        {PREDICT_PARAM_STREAM, QVariant(stream)},
        {PREDICT_PARAM_TEMPERATURE, QVariant(temperature)},
        {PREDICT_PARAM_NOJSONOUTPUT, QVariant(true)}
    };
    const QPair<AIServer::ErrorType, QStringList> &chatText = m_chatSession->requestChatText(llmId, conversation, params);
    if (chatText.first != AIServer::NoError) {
        qCWarning(logDBus) << "Failed to request chat text:" << chatText.second.value(1);
        sendErrorReply(QDBusError::NoServer, chatText.second.value(1));
        return QStringList();
    }

    return chatText.second;
}

bool AppDbusObject::setCurrentLLMAccountId(const QString &id)
{
    if (!checkAgreement())
        return false;

    qCDebug(logDBus) << "Setting current LLM account ID:" << id;
    return m_chatSession->setCurrentLLMAccountId(id);
}

QString AppDbusObject::currentLLMAccountId()
{
    if (!checkAgreement())
        return QString();

    QString id = m_chatSession->currentLLMAccountId();
    qCDebug(logDBus) << "Current LLM account ID:" << id;
    return id;
}

QString AppDbusObject::queryLLMAccountList()
{
    if (!checkAgreement())
        return QString();

    QList<LLMChatModel> excludes;
    excludes << LLMChatModel::LOCAL_TEXT2IMAGE;
    return m_chatSession->queryUosAiLLMAccountList();
}

void AppDbusObject::launchLLMUiPage(bool showAddllmPage)
{
    qCDebug(logDBus) << "Launching LLM UI page, showAddllmPage:" << showAddllmPage;
    emit launchUI(showAddllmPage);
}

bool AppDbusObject::checkAgreement()
{
    if (WelcomeDialog::isAgreed())
        return true;

    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(logDBus) << "Request not in main thread, cannot show WelcomeDialog";
        return false;
    }

    qCDebug(logDBus) << "Showing WelcomeDialog";
    WelcomeDialog::instance(false)->exec();
    return WelcomeDialog::isAgreed();
}
