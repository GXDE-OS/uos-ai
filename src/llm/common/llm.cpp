#include "llm.h"
#include "networkdefs.h"

LLM::LLM(const LLMServerProxy &serverproxy)
    : m_accountProxy(serverproxy)
{

}

LLM::~LLM()
{

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

QList<QByteArray> LLM::text2Image(const QString &prompt, int number)
{
    m_lastError = AIServer::ContentAccessDenied;
    m_lastErrorString = QCoreApplication::translate("LLM", "Vincent picture service is not supported");
    return QList<QByteArray>();
}
