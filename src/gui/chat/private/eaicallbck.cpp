#include "eaicallbck.h"
#include "uosai_global.h"
#include "../Instruction/instructionmanager.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

UOSAI_USE_NAMESPACE

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
    qCDebug(logAIGUI) << "Setting notifier:" << notifyName;
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
        qCWarning(logAIGUI) << "Invalid callback parameter:"
                   << " op=" << op
                   << " owner=" << owner
                   << " notifier =" << notifier;
    }
}

EAiCacheCallback::EAiCacheCallback(QObject *caller)
    : EAiCallback(caller)
{
    mode = StreamMode;
}

void EAiCacheCallback::notify(const QString &aiReply, int err)
{
    Q_UNUSED(aiReply)
    Q_UNUSED(err)
}

EAiStreamCallback::EAiStreamCallback(QObject *caller)
    : EAiCallback(caller)
{
    mode = StreamMode;
}

EAiInstructionCallback::EAiInstructionCallback(QObject *caller)
    : EAiCallback(caller)
{

}

void EAiInstructionCallback::setInstType(int type)
{
    inst = type;
    qCDebug(logAIGUI) << "Setting instruction type:" << type;
}

int EAiInstructionCallback::instType()
{
    return inst;
}

void EAiInstructionCallback::notify(const QString &aiReply, int err)
{
    if (err != 0 && err != 200) {
        qCWarning(logAIGUI) << "Instruction callback error:" << err;
        sendNotify(aiReply, err);
        return;
    }

    if (inst == InstructionManager::SearchOnline) {
        // 非流式 err = 0
        QString instRunResult = InstructionManager::instance()->onAiCallback(inst, aiReply);
        sendNotify(instRunResult, AnswerOperateType::OpenURL);
    } else if (inst != InstructionManager::None) {
        // 流式 - streamEnd时处理指令
        if (err == 0) {
            QString content;
            auto obj = QJsonDocument::fromJson(aiReply.toUtf8()).object();
            if (obj.contains("message")) {
                auto msg = obj.value("message").toObject();
                if (msg.value("chatType").toInt() == ChatTextPlain) {
                    content = msg.value("content").toString();
                }
            }
            reply += content;
        }
        else if (err == 200) {
            QString instRunResult = InstructionManager::instance()->onAiCallback(inst, reply);
            sendNotify(instRunResult, 0);
            sendNotify(QString(""), 200);
        }
    }
}

void EAiInstructionCallback::sendNotify(const QString &content, int err)
{
    if (nullptr != owner && !notifier.isEmpty()) {
        QMetaObject::invokeMethod(
            owner,
            notifier.toStdString().c_str(),
            Qt::AutoConnection,
            Q_ARG(int, op),
            Q_ARG(QString, content),
            Q_ARG(int, err));
    } else {
        qCWarning(logAIGUI) << "Invalid callback parameter:"
                   << " op=" << op
                   << " owner=" << owner
                   << " notifier =" << notifier;
    }
}
