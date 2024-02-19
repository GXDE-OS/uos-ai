#include "threadtaskmana.h"
#include "llm.h"

LLMThreadTaskMana::LLMThreadTaskMana()
{

}

LLMThreadTaskMana *LLMThreadTaskMana::instance()
{
    static LLMThreadTaskMana manager;
    return &manager;
}

void LLMThreadTaskMana::addRequestTask(const QString &id, QWeakPointer<LLM> llm)
{
    m_runingRequestTask[id] = llm;
}

void LLMThreadTaskMana::requestTaskFinished(const QString &id)
{
    m_runingRequestTask.remove(id);
}

void LLMThreadTaskMana::cancelRequestTask(const QString &id)
{
    QWeakPointer<LLM> copilot = m_runingRequestTask.value(id);
    if (!copilot.isNull())
        copilot.toStrongRef()->cancel();
}

bool LLMThreadTaskMana::isFinished(const QString &id)
{
    return !m_runingRequestTask.contains(id);
}

QThreadPool *LLMThreadTaskMana::threadPool(const QString &key)
{
    if (!m_threadPools.contains(key) || m_threadPools[key].isNull()) {
        QThreadPool *threadPool = new QThreadPool;
        threadPool->setMaxThreadCount(5);
        m_threadPools[key].reset(threadPool);
    }

    return m_threadPools[key].data();
}
