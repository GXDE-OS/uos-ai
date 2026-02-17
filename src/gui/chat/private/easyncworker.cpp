#include "easyncworker.h"

#include <QEventLoop>
#include <QTimer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

static int TimeOutMs = 2000;

EAsyncWorker::EAsyncWorker(QObject *parent)
{
}

bool EAsyncWorker::executeSynchronouslyGeneric(AsyncWorkerType type)
{
    qCDebug(logAIGUI) << "Executing generic async task, type:" << type;
    emit sigAsyncWorker(type);

    QEventLoop loop;
    QTimer timer;
    bool success = false;

    QMetaObject::Connection conn = QObject::connect(this, &EAsyncWorker::sigWorkerFinished, this, [&loop, &success, type](int finishedType){
        if (type == finishedType) {
            success = true;
            qCInfo(logAIGUI) << "Async task completed successfully, type:" << type;
            loop.quit();
        }
    });
    QObject::connect(&timer, &QTimer::timeout, [&loop, conn]() {
        loop.quit();
    });
    timer.start(TimeOutMs);

    loop.exec(QEventLoop::ProcessEventsFlag::EventLoopExec);
    disconnect(conn);

    if (!success) {
        qCWarning(logAIGUI) << "Async task timeout or failed, type:" << type;
        return false;
    }

    return true;
}

bool EAsyncWorker::chatInitSyncWorker()
{
    QEventLoop loop;
    QTimer timer;
    bool success = false;

    QMetaObject::Connection conn = QObject::connect(this, &EAsyncWorker::sigChatInitSyncWorkerFinished, this, [&loop, &success](){
        success = true;
        qCInfo(logAIGUI) << "Chat initialization async task completed successfully";
        loop.quit();
    });
    QObject::connect(&timer, &QTimer::timeout, [&loop]() {
        loop.quit();
    });
    timer.start(TimeOutMs);

    loop.exec(QEventLoop::ProcessEventsFlag::EventLoopExec);
    disconnect(conn);

    if (!success) {
        qCWarning(logAIGUI) << "Chat initialization async task timeout or failed";
        return false;
    }

    return true;
}
