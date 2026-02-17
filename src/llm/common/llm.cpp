#include "llm.h"
#include "networkdefs.h"

LLM::LLM(const LLMServerProxy &serverproxy)
    : m_accountProxy(serverproxy)
{

}

LLM::~LLM()
{

}

void LLM::updateAccount(const LLMServerProxy &serverproxy)
{
    m_accountProxy = serverproxy;
}

void LLM::loadParams(const QVariantHash &params)
{
    m_params = params;
}

void LLM::cancel()
{
    emit aborted();
}

void LLM::setCreatedId(const QString &id)
{
    m_createdId = id;
}

QString LLM::createdId() const
{
    return m_createdId;
}

void LLM::switchStream(bool on)
{
    m_streamSwitch = on;
}

bool LLM::stream() const
{
    return m_streamSwitch;
}

int LLM::lastError() const
{
    return m_lastError;
}

void LLM::setLastError(int error)
{
    m_lastError = error;
}

QString LLM::lastErrorString()
{
    return m_lastErrorString;
}

void LLM::setLastErrorString(const QString &errorMessage)
{
    m_lastErrorString = errorMessage;
}

bool LLM::isReplied() const
{
    return m_replied;
}

QList<QByteArray> LLM::text2Image(const QString &prompt, int number)
{
    m_lastError = AIServer::ContentAccessDenied;
    m_lastErrorString = QCoreApplication::translate("LLM", "Vincent picture service is not supported");
    return QList<QByteArray>();
}

void LLM::readyThinkChainContent(const QJsonObject &content)
{
    if (content.contains("reasoningContent")) {
        QJsonObject message;
        QJsonObject wrapper;
        message.insert("content", content.value("reasoningContent").toString());
        message.insert("chatType", ChatAction::ChatTextThink);  // Think 类型
        wrapper.insert("message", message);
        wrapper.insert("stream", m_streamSwitch);
        emit readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
    }

    if (content.contains("content")) {
        QJsonObject message;
        QJsonObject wrapper;
        message.insert("content", content.value("content").toString());
        message.insert("chatType", ChatAction::ChatTextPlain);  // 普通文本类型
        wrapper.insert("message", message);
        wrapper.insert("stream", m_streamSwitch);
        emit readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
    }
}

void LLM::textChainContent(const QString &content)
{
    QJsonObject message;

    message.insert("content", content);
    message.insert("chatType", ChatAction::ChatTextPlain);  // 普通文本类型

    QJsonObject wrapper;
    wrapper.insert("message", message);
    wrapper.insert("stream", stream());

    emit readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
}

LLMServerProxy LLM::account() const
{
    return m_accountProxy;
}
