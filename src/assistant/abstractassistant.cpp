#include "abstractassistant.h"
#include "conversation/conversationmanager.h"
#include "global_key_define.h"

using namespace uos_ai;

AbstractAssistant::AbstractAssistant(QObject *parent)
    : QObject(parent)
{
}

AbstractAssistant::~AbstractAssistant()
{
}

void AbstractAssistant::setModelId(const QString &modelId)
{
    m_modelId = modelId;
}

QString AbstractAssistant::modelId() const
{
    return m_modelId;
}

void AbstractAssistant::setConversation(const QSharedPointer<ConversationRecord> &conv)
{
    m_conversation = conv;
}

QSharedPointer<ConversationRecord> AbstractAssistant::conversation() const
{
    return m_conversation;
}

bool AbstractAssistant::initialize(const QVariantHash &parameters)
{
    m_parameters = parameters;
    return true;
}

QVariantHash AbstractAssistant::execute()
{
    emit started();

    m_error.clear();

    if (m_modelId.isEmpty()) {
        m_error[STR_KEY_ERROR] = GErrorType::InvalidModel;
        m_error[STR_KEY_ERROR_MESSAGE] = tr("UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
        emit finished({});
        return m_error;
    }

    if (m_conversation.isNull()) {
        m_error[STR_KEY_ERROR] = GErrorType::InvalidAssistant;
        emit finished({});
        return m_error;
    }

    auto ret = run();

    emit finished(ret);
    return ret;
}

void AbstractAssistant::invokeAction(const QJsonObject &action)
{
    Q_UNUSED(action);
    return;
}

QVariantHash AbstractAssistant::lastError() const
{
    return m_error;
}
