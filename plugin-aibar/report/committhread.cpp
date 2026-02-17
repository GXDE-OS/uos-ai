#include "committhread.h"
#include <QLoggingCategory>
#include <QLibrary>

Q_DECLARE_LOGGING_CATEGORY(logAIBar)

#include <QDebug>

UOSAI_USE_NAMESPACE

using namespace report;

CommitLog::CommitLog(QObject *parent)
    : QObject(parent)
{
}

CommitLog::~CommitLog()
{
}

void CommitLog::commit(const QString &data)
{
    if (data.isEmpty()) {
        qCDebug(logAIBar) << "Empty data received, skipping commit";
        return;
    }

    qCDebug(logAIBar) << "Committing event log data";
    m_writeEventLog(data.toStdString());
}

bool CommitLog::init()
{
    qCDebug(logAIBar) << "Initializing event log library";
    QLibrary library("deepin-event-log");
    if (!library.load()) {
        qCWarning(logAIBar) << "Failed to load deepin-event-log library";
        return false;
    }

    m_initEventLog = reinterpret_cast<InitEventLog>(library.resolve("Initialize"));
    m_writeEventLog = reinterpret_cast<WriteEventLog>(library.resolve("WriteEventLog"));

    if (!m_initEventLog || !m_writeEventLog) {
        qCWarning(logAIBar) << "Failed to resolve required library functions";
        return false;
    }

    if (!m_initEventLog("uos-ai", false)) {
        qCWarning(logAIBar) << "Initialize function call failed";
        return false;
    }

    qCDebug(logAIBar) << "Event log library initialized successfully";
    return true;
}
