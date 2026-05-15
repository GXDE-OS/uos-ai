#include "global_key_define.h"
#include "sessionmanager.h"
#include "assistant/assistantmanager.h"

#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QThread>
#include <QApplication>
#include <QLoggingCategory>
#include <QtConcurrent>

Q_DECLARE_LOGGING_CATEGORY(logSession)
using namespace uos_ai;

QMap<QString, QSharedPointer<SessionManager>> SessionManager::instances;
QReadWriteLock SessionManager::inslock;

QSharedPointer<SessionManager> SessionManager::instance(const QString &app)
{
    inslock.lockForRead();
    QSharedPointer<SessionManager> sm = instances.value(app, nullptr);
    inslock.unlock();

    if (sm.isNull()) {
        inslock.lockForWrite();
        sm = QSharedPointer<SessionManager>(new SessionManager());
        instances.insert(app, sm);
        inslock.unlock();
    }

    return sm;
}

void SessionManager::destroyInstance(const QString &app)
{
    inslock.lockForWrite();
    QSharedPointer<SessionManager> sm = instances.value(app, nullptr);
    if (!sm.isNull()) {
        instances.remove(app);
    }
    inslock.unlock();
}

QJsonObject SessionManager::createSession(const QString &assistId, const QString &sessionId)
{
    QJsonObject ret;
    QString id = sessionId.isEmpty() ? GlobalUtil::generateMsId() : sessionId;
    if (sessions.contains(id)) {
        qCWarning(logSession) << "Session with ID" << id << "already exists";
        ret[STR_KEY_ERROR] = GErrorType::InvalidSession;
        ret[STR_KEY_MESSAGE] = tr("Session with ID %1 already exists").arg(id);
        return ret;
    }

    // 创建Session
    SessionPtr obj(new BaseSession(id));
    connect(obj.data(), &BaseSession::sessionEvent, this, &SessionManager::onEvent);

    auto assistantPtr = AssistantMgr->createAssistant(assistId);
    if (assistantPtr.isNull()) {
        ret[STR_KEY_ERROR] = GErrorType::InvalidSession;
        ret[STR_KEY_MESSAGE] = tr("Session %1 create assistant %2 failed").arg(sessionId).arg(assistId);
        qCWarning(logSession) << ret[STR_KEY_MESSAGE].toString();
        return ret;
    }

    obj->setAssistant(assistantPtr);

    sessions.insert(id, obj);

    ret[STR_KEY_ERROR] = GErrorType::NoError;
    ret[STR_KEY_ID] = id;
    ret[STR_KEY_MESSAGE] = tr("Session %1 create success").arg(id);

    return ret;
}

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    releaseTimer.setSingleShot(true);
    releaseTimer.setInterval(3000);
    connect(&releaseTimer, &QTimer::timeout, this, [this]() {
        qCDebug(logSession) << "release session" << delayRelease.size();
        this->delayRelease.clear();
    });

    threadPool.setMaxThreadCount(10);
}

SessionManager::~SessionManager()
{
    sessions.clear();
    delayRelease.clear();
}

SessionPtr SessionManager::getSession(const QString &sessionId) const
{
    return sessions.value(sessionId, nullptr);
}

bool SessionManager::removeSession(const QString &sessionId)
{
    if (!sessions.contains(sessionId)) {
        return false;
    }

    delayRelease.append(sessions.take(sessionId));
    releaseTimer.start();
    return true;
}

QJsonObject SessionManager::runSession(const QString &sessionId, const QString &model, const QSharedPointer<ConversationRecord> &conversation, const QVariantHash &parameters)
{
    QJsonObject ret;
    if (!sessions.contains(sessionId)) {
        ret[STR_KEY_ERROR] = GErrorType::InvalidSession;
        ret[STR_KEY_MESSAGE] = tr("Session %1 not found").arg(sessionId);
        return ret;
    }

    auto se = sessions.value(sessionId);

    if (se->state() != SessionState::SsIdle) {
        ret[STR_KEY_ERROR] = GErrorType::InvalidSession;
        ret[STR_KEY_MESSAGE] = tr("Session %1 is not idle").arg(sessionId);
        removeSession(sessionId);
        return ret;
    }

    auto assistPtr = se->assistant();
    Q_ASSERT(!assistPtr.isNull());

    assistPtr->setModelId(model);
    assistPtr->setConversation(conversation);

#ifdef COMPILE_ON_QT6
    QFuture<QJsonObject> future = QtConcurrent::run(&threadPool, &BaseSession::run, se.data(), parameters);
#else
    QFuture<QJsonObject> future = QtConcurrent::run(&threadPool, se.data(), &BaseSession::run, parameters);
#endif
    QFutureWatcher<QJsonObject> *futureWatcher = new QFutureWatcher<QJsonObject>(se.data());

    futureWatcher->setFuture(future);

    return ret;
}

QStringList SessionManager::getAllSessionIds() const
{
    return sessions.keys();
}

int SessionManager::sessionCount() const
{
    return sessions.size();
}

void SessionManager::cancelSession(const QString &sessionId)
{
    auto session = sessions.value(sessionId);
    if (session)
        session->cancel();
}

void SessionManager::onEvent(int event, const QString &id, const QString &json)
{
    emit sessionEvent(event, id, json);
    if (event == SessionEvent::SeFinished) {
        removeSession(id);
    }
}
