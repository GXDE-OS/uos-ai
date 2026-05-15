#include "global_key_define.h"
#include "basesession.h"
#include "assistant/assistantmanager.h"
#include "conversation/conversationrecord.h"
#include "conversation/conversationmanager.h"
#include "model/modelvendor.h"
#include "network/httpcodetranslation.h"
#include "services/accountservice/freeaccountservice.h"
#include "database/appdatabase.h"

#include <QJsonDocument>
#include <QLoggingCategory>
#include <QUuid>

Q_DECLARE_LOGGING_CATEGORY(logSession)

using namespace uos_ai;

BaseSession::BaseSession(const QString &sessionId, QObject *parent)
    : QObject(parent)
    , m_id(sessionId)
{
}

BaseSession::~BaseSession()
{
}

SessionState BaseSession::state() const
{
    return m_state;
}

void BaseSession::setAssistant(const AssistantPtr &assist)
{
    m_assistant = assist;

    connect(m_assistant.data(), &AbstractAssistant::started, this, &BaseSession::onStarted);
    connect(m_assistant.data(), &AbstractAssistant::finished, this, &BaseSession::onFinished);
    connect(m_assistant.data(), &AbstractAssistant::pushMessage, this, &BaseSession::receiveMessage);
}

AssistantPtr BaseSession::assistant() const
{
    return m_assistant;
}

void BaseSession::cancel()
{
    if (m_assistant.isNull()) 
        return;

    m_assistant->cancel();
}

QString BaseSession::id() const
{
    return m_id;
}

QJsonObject BaseSession::run(const QVariantHash &parameters)
{
    qCInfo(logSession) << "session" << m_id << "run.";
    {
        if (!AppDatabase::instance()->getConfigBool(CONFIG_APP_AGREEMENT)) {
            qCCritical(logSession) << "App agreement not accepted, aborting request";
            abort();
            return {};
        }

        // 免费账号检查使用次数
        auto error = FreeAccountService::instance()->validateUsage(m_assistant->modelId());
        if (!error.isEmpty()) {
            // 按流程先发start
            onStarted();

            onErrorFinished(error);
            return {};
        }
    }

    m_state = SessionState::SsReady;
    m_assistant->initialize(parameters);
    Q_ASSERT(m_assistant);

    return QJsonObject::fromVariantHash(m_assistant->execute());
}

void BaseSession::receiveMessage(const QString &message)
{
    emit sessionEvent(SeMessage, m_id, message);
}

void BaseSession::onFinished(const QVariantHash &result)
{
    QList<ModelMessage> msg = result.value(STR_KEY_CONTEXT).value<QList<ModelMessage>>();
    QString id = GlobalUtil::generateMsId();
    auto conv = m_assistant->conversation();
    {
        MessageNodePtr msgNode(new MessageNode);
        msgNode->setId(id);
        msgNode->setRole(MrAssistant);
        msgNode->setModelId(m_assistant->modelId());
        msgNode->setMessage(msg);
        conv->addMessage(conv->currentMessage(), msgNode);

        if (!msg.isEmpty())
            FreeAccountService::instance()->incrementUsage(m_assistant->modelId());

    }

    if (!m_assistant->lastError().isEmpty()) {
        QJsonObject error = QJsonObject::fromVariantHash(m_assistant->lastError());
        QString errorJson = QJsonDocument(error).toJson(QJsonDocument::Compact);
        emit sessionEvent(SeError, m_id, errorJson);
    }

    qCInfo(logSession) << "session" << m_id << "finished, error:" << m_assistant->lastError();

    QJsonObject content;
    content[STR_KEY_ID] = id;

    m_state = SessionState::SsFinished;
    emit sessionEvent(SeFinished, m_id, QString::fromUtf8(QJsonDocument(content).toJson(QJsonDocument::Compact)));
}

void BaseSession::onStarted()
{
    qCInfo(logSession) << "session" << m_id << "started";
    m_state = SessionState::SsRunning;
    emit sessionEvent(SeStarted, m_id, QString());
}

void BaseSession::onErrorFinished(const QVariantHash &error)
{
    qCWarning(logSession) << "session" << m_id << "finished by error" << error;
    QString id = GlobalUtil::generateMsId();
    auto conv = m_assistant->conversation();
    {
        MessageNodePtr msgNode(new MessageNode);
        msgNode->setId(id);
        msgNode->setRole(MrAssistant);
        msgNode->setModelId(m_assistant->modelId());
        conv->addMessage(conv->currentMessage(), msgNode);
    }

    QString errorJson = QJsonDocument(QJsonObject::fromVariantHash(error)).toJson(QJsonDocument::Compact);
    emit sessionEvent(SeError, m_id, errorJson);

    QJsonObject content;
    content[STR_KEY_ID] = id;

    m_state = SessionState::SsFinished;
    emit sessionEvent(SeFinished, m_id, QString::fromUtf8(QJsonDocument(content).toJson(QJsonDocument::Compact)));
    return ;
}
