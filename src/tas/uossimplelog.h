#ifndef UOSSIMPLELOG_H
#define UOSSIMPLELOG_H

#include "tlockfreequeue.h"

#include <QQueue>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QDateTime>

namespace uos_ai {

enum UosLogType {
    UserInput = 1,
    FailedRetry,
    TextToImageResult,
    AnwserRate
};

struct UosLogObject {
    UosLogType type;  // 日志类型
    QString app;
    QString content;
    QDateTime time;
    QString llm;
    QString ModelType;
    QString assistant;
    int t2iResult; // 0成功 1失败
} ;

struct UosRateLog {
    enum Rate{
        None = -1,
        Like = 1,
        Dislike = 2,
        Cancle = 3
    };

    UosLogType type;  // 日志类型
    QString question;
    QString answer;
    QString questionTime;
    QString answerTime;
    QString llm;
    QString app;
    QString modelType;
    QString assistantName;
    Rate likeOrNot = None;
};

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

}

#endif // UOSSIMPLELOG_H
