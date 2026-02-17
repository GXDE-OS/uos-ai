#include "committhread.h"

#include <QLibrary>
#include <QDebug>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logUtils)

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
    if (data.isEmpty())
        return;

    //qDebug() << data.toStdString().c_str();
    m_writeEventLog(data.toStdString());
}

bool CommitLog::init()
{
#ifdef QT_DEBUG
    qCDebug(logUtils) << "Debug mode";
    return false;
#else
    QLibrary library("deepin-event-log");
    if (!library.load()) {
        qCWarning(logUtils) << "Load library failed";
        return false;
    }

    m_initEventLog = reinterpret_cast<InitEventLog>(library.resolve("Initialize"));
    m_writeEventLog = reinterpret_cast<WriteEventLog>(library.resolve("WriteEventLog"));

    if (!m_initEventLog || !m_writeEventLog) {
        qCWarning(logUtils) << "Library init failed";
        return false;
    }

    if (!m_initEventLog("uos-ai", false)) {
        qCWarning(logUtils) << "Initialize called failed";
        return false;
    }

    return true;
#endif
}
