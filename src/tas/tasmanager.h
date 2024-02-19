#ifndef TASMANAGER_H
#define TASMANAGER_H
#include "tasdef.h"

#include <QString>
#include <QByteArray>
#include <QQueue>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QSharedPointer>

class TAS;

//#define OpenTextAuditService

class TasManager: public QThread
{
    Q_OBJECT
public:
    TasManager();

    ~TasManager();

    void setTas(QSharedPointer<TAS> tas);

    void auditText(const QByteArray &);

    void endAuditText();

    QByteArray getNormalText() const;

    void run() override ;

    void stopAuditing();

    bool auditFinished();

    QSharedPointer<TextAuditResult> getResult();

    static QString clearPunct(const QByteArray &buff);

signals:
    void sigReadyAuditContent(QByteArray content);

    void sigAuditContentResult(QSharedPointer<TextAuditResult> result);

private:
    bool matchPunct(const QChar &ch);

private:
    QSharedPointer<TAS> m_tas;

    std::atomic<bool> m_stopAuditing;

    std::atomic<bool> m_auditFinished;

    QSharedPointer<TextAuditResult> m_result;

    QByteArray m_auditingBuff;

    QQueue<QByteArray> m_auditBuffQueue;

    QMutex m_mutex;

    QWaitCondition m_condition;

};


#endif // TASMANAGER_H
