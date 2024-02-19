#ifndef THREADTASKMANA_H
#define THREADTASKMANA_H

#include <QMap>
#include <QThreadPool>
#include <QSharedPointer>

class LLM;
class LLMThreadTaskMana : public QObject
{
    Q_OBJECT
public:
    LLMThreadTaskMana();
    static LLMThreadTaskMana *instance();

public:
    /**
     * @brief addRequestTask
     * @param id
     * @param llm
     */
    void addRequestTask(const QString &id, QWeakPointer<LLM> llm);

    /**
     * @brief requestTaskFinished
     * @param id
     */
    void requestTaskFinished(const QString &id);

    /**
     * @brief cancelRequestTask
     * @param id
     */
    void cancelRequestTask(const QString &id);

    /**
     * @brief isFinished
     * @param id
     * @return
     */
    bool isFinished(const QString &id);

public:
    /**
     * @brief threadPool
     * @return
     */
    QThreadPool *threadPool(const QString &key);

private:
    QMap<QString, QWeakPointer<LLM>> m_runingRequestTask;
    QMap<QString, QSharedPointer<QThreadPool>> m_threadPools;
};

#endif // THREADTASKMANA_H
