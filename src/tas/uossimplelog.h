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

    void addRateLog(const UosRateLog &logObj);
    static QJsonObject toJson(const UosRateLog &logObj);

protected:
    void run() override ;

private:
    UosSimpleLog(QObject *parent = nullptr);

    int pushLog(const UosLogObject &logObj);
    int pushLog(const UosRateLog &logObj);

    int sendLog(const QByteArray &sendData, UosLogType logType);
    QString simplifiedText(const QString &content);

    QString hostUrl(UosLogType type = UosLogType::UserInput) const;

    void initServerAddress();

private:
    TLockFreeQueue<UosLogObject> m_preLogObjectQueue;
    TLockFreeQueue<UosRateLog> m_preRateLogQueue;

    QMutex m_mutex;

    QWaitCondition m_condition;

    std::atomic<bool> m_stopLogging;

    QMap<UosLogType, QString> serverUrls;
};

#endif // UOSSIMPLELOG_H
