#include "eaicallbck.h"

#include <QDebug>

EAiCallback::EAiCallback(QObject *caller)
    : owner(caller)
{
}

EAiCallback::~EAiCallback()
{
}

bool EAiCallback::isStreamMode()
{
    return mode == StreamMode;
}

void EAiCallback::setOwner(QObject *caller)
{
    owner = caller;
}

QObject *EAiCallback::getOwner()
{
    return owner;
}

void EAiCallback::setNotifier(QString notifyName)
{
    notifier = notifyName;
}

void EAiCallback::setDataMode(DataMode m)
{
    if (m > None && m < MaxMode) {
        mode = m;
    }
}

void EAiCallback::setOp(int act)
{
    op = act;
}

void EAiCallback::notify(const QString &aiReply, int err)
{
    if (nullptr != owner && !notifier.isEmpty()) {
        QMetaObject::invokeMethod(
            owner,
            notifier.toStdString().c_str(),
            Qt::AutoConnection,
            Q_ARG(int, op),
            Q_ARG(QString, aiReply),
            Q_ARG(int, err));
    } else {
        qWarning() << "Invalid callback parameter:"
                   << " op=" << op
                   << " owner=" << owner
                   << " notifier =" << notifier;
    }
}

EAiCacheCallback::EAiCacheCallback(QObject *caller)
    : EAiCallback(caller)
{
    mode = CacheMode;
}

EAiStreamCallback::EAiStreamCallback(QObject *caller)
    : EAiCallback(caller)
{
    mode = StreamMode;
}
