#ifndef UOSSIMPLELOG_H
#define UOSSIMPLELOG_H
#include "tasdef.h"
#include "tlockfreequeue.h"

#include <QQueue>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

class UosSimpleLog : public QThread
{
    Q_OBJECT
public:
    ~UosSimpleLog();

    static UosSimpleLog &instance();

    void addLog(const UosLogObject &logObj);

protected:
    void run() override ;

private:
    UosSimpleLog(QObject *parent = nullptr);

    int pushLog(const UosLogObject &logObj);

    QString simplifiedText(const QString &content);

    QString hostUrl(UosLogType type = UosLogType::UserInput) const;

private:
    TLockFreeQueue<UosLogObject> m_preLogObjectQueue;

    QMutex m_mutex;

    QWaitCondition m_condition;

    std::atomic<bool> m_stopLogging;
};

#endif // UOSSIMPLELOG_H
