#include "eventlogutil.h"
#include "committhread.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIBar)

#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>
#include <QDebug>

UOSAI_USE_NAMESPACE
using namespace report;

class EventLogUtilGlobal : public EventLogUtil
{
};
Q_GLOBAL_STATIC(EventLogUtilGlobal, eventLogUtilGlobal)

EventLogUtil::EventLogUtil(QObject *parent)
    : QObject(parent)
{
    init();
}

EventLogUtil::~EventLogUtil()
{
    qCDebug(logAIBar) << "Destroying EventLogUtil";
    if (m_commitThread) {
        m_commitThread->quit();
        if (!m_commitThread->wait(2000)) {
            qCWarning(logAIBar) << "Failed to stop commit thread gracefully";
        }
    }
}

EventLogUtil *EventLogUtil::instance()
{
    return eventLogUtilGlobal;
}

void EventLogUtil::writeEvent(const QVariantMap &data)
{
    if (!m_isInit) {
        qCDebug(logAIBar) << "Event logging not initialized, skipping write";
        return;
    }

    qCDebug(logAIBar) << "Writing event log with" << data.size() << "data items";
    emit appendData(QJsonDocument(castToJson(data)).toJson(QJsonDocument::Compact));
}

QJsonObject EventLogUtil::castToJson(const QVariantMap &data) const
{
    QJsonObject json;
    for (auto it = data.begin(); it != data.end(); ++it) {
        json.insert(it.key(), it.value().toJsonValue());
    }
    return json;
}

void EventLogUtil::init()
{
    if (m_isInit) {
        qCDebug(logAIBar) << "Already initialized";
        return;
    }

    qCDebug(logAIBar) << "Setting up event logging system";
    m_commitLog = new CommitLog();
    if (!m_commitLog->init()) {
        qCWarning(logAIBar) << "Failed to initialize commit log";
        return;
    }

    m_commitThread = new QThread();
    connect(this, &EventLogUtil::appendData, m_commitLog, &CommitLog::commit);
    connect(m_commitThread, &QThread::finished, [&]() {
        qCDebug(logAIBar) << "Commit thread finished";
        m_commitLog->deleteLater();
    });
    m_commitLog->moveToThread(m_commitThread);
    m_commitThread->start();

    m_isInit = true;
    qCDebug(logAIBar) << "Event logging system initialized successfully";
}
