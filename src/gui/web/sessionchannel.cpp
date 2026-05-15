#include "sessionchannel.h"
#include "session/sessionmanager.h"
#include "conversation/conversationmanager.h"
#include "global_key_define.h"
#include "utils/util.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

SessionChannel::SessionChannel(QObject *parent)
    : QObject(parent)
{
    connect(WebSessMgr().get(), &SessionManager::sessionEvent, this, &SessionChannel::sessionEvent);
}

SessionChannel::~SessionChannel()
{
}

void SessionChannel::sendMessage(const QString &params)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "sendMessage error: " << err.errorString();

        Q_ASSERT(err.error != QJsonParseError::NoError);
        return;
    }

    auto root = doc.object();
    QString sessionId = root[STR_KEY_SESSION_ID].toString();
    QString conversationId = root[STR_KEY_CONVERSATION_ID].toString();
    if (sessionId.isEmpty() || conversationId.isEmpty()) {
        qCWarning(logAIGUI) << "sendMessage error: sessionId or conversationId is empty";

        Q_ASSERT(!sessionId.isEmpty());
        Q_ASSERT(!conversationId.isEmpty());
        return;
    }

    QString assist = root.value(STR_KEY_ASSISTANT).toString();
    QString model = root.value(STR_KEY_MODEL).toString();
    QString modelName = root.value(STR_KEY_MODEL_NAME).toString();

    // 处理聊天记录
    {
        auto msgs = root[STR_KEY_MESSAGE].toObject();
        QString pre = msgs.value(STR_KEY_PREVIOUS).toString();
        QString currentMsgId = msgs.value(STR_KEY_ID).toString();
        Q_ASSERT(!currentMsgId.isEmpty());

        MessageNodePtr msgNode(new MessageNode);
        msgNode->setId(currentMsgId);
        msgNode->setRole(MrUser);
        msgNode->setModelName(modelName);
        msgNode->setModelId(model);
        const QVariantHash extension = msgs.value(STR_KEY_EXTENSION).toObject().toVariantHash();
        if (!extension.isEmpty())
            msgNode->setExtension(extension);
        currentMsgId = msgNode->getId();

        ModelMessage modelMsg;
        modelMsg.role = STR_KEY_USER;

        ConversationRecordPtr record = ConvMgr()->getConversation(conversationId);
        bool isNewConversation = false;
        if (record) {
            qCDebug(logAIGUI) << "continue conversation" << conversationId;
        } else {
            qCDebug(logAIGUI) << "start new conversation" << conversationId;
            record = ConvMgr()->createConversation(conversationId);
            record->setAssistantId(assist);
            isNewConversation = true;
        }
        record->setModelId(model);

        const QJsonArray contents = msgs.value(STR_KEY_CONTENT).toArray();
        for (const auto &val : contents) {
            if (!val.isObject())
                continue;

            QJsonObject contentObj = val.toObject();
            QString typeStr = contentObj.value(STR_KEY_TYPE).toString();
            ContentType type = GlobalUtil::contentTypeFromString(typeStr);
            QVariant data;

            if (type == CntText) {
                data = contentObj.value(STR_KEY_DATA).toObject().value(STR_KEY_CONTENT).toString();
            } else if (type == CntImage) {
                data = contentObj.value(STR_KEY_DATA).toObject().value(STR_KEY_CONTENT).toVariant().toHash();
            } else if (type == CntFile) {
                QVariantList paths = contentObj.value(STR_KEY_DATA).toArray().toVariantList();
                if (paths.isEmpty())
                    continue;

                // model message: list of paths for the assistant to read content
                MetaMessage metaMsg;
                metaMsg.type = CntFile;
                metaMsg.data = paths;
                modelMsg.content.append(metaMsg);

                // render: one entry per file with filePath
                for (int i = 0; i < paths.size(); ++i) {
                    QVariantHash fileData;
                    fileData[STR_KEY_FILE_PATH] = paths[i].toString();
                    fileData[STR_KEY_INDEX] = i;
                    RenderMessage fileRender;
                    fileRender.type = CntFile;
                    fileRender.data = fileData;
                    msgNode->appendRender(fileRender);
                }
                continue;
            } else {
                continue;
            }

            RenderMessage renderMsg;
            renderMsg.type = type;
            renderMsg.data = data;
            msgNode->appendRender(renderMsg);

            MetaMessage metaMsg;
            metaMsg.type = type;
            metaMsg.data = data;
            modelMsg.content.append(metaMsg);

            // Set conversation title from first text message when starting new conversation
            if (isNewConversation && type == CntText) {
                QString title = data.toString();
                if (!title.isEmpty()) {
                    record->setTitle(title);
                }
                isNewConversation = false;
            }
        }
        msgNode->appendMessage(modelMsg);

        record->addMessage(pre, msgNode);
        record->setCurrentMessage(currentMsgId);
    }

    // 创建会话
    {
        auto obj = WebSessMgr()->createSession(assist, sessionId);
        if (obj.value(STR_KEY_ID).toString() != sessionId) {
            qCWarning(logAIGUI) << "createSession error: " << obj.value(STR_KEY_MESSAGE).toString();
            QMetaObject::invokeMethod(this, "sessionEvent", Qt::QueuedConnection,
                Q_ARG(int , SessionEvent::SeError), Q_ARG(QString, sessionId), Q_ARG(QString, ""));

            Q_ASSERT(obj.value(STR_KEY_ID).toString() == sessionId);
            return;
        }
    }

    auto runRet = WebSessMgr()->runSession(sessionId, model, ConvMgr()->getConversation(conversationId), root.value(STR_KEY_PARAMS).toObject().toVariantHash());
    if (runRet.contains(STR_KEY_ERROR)) {
        qCWarning(logAIGUI) << "runSession error: " << runRet.value(STR_KEY_MESSAGE).toString();
        QMetaObject::invokeMethod(this, "sessionEvent", Qt::QueuedConnection,
            Q_ARG(int , SessionEvent::SeError), Q_ARG(QString, sessionId), Q_ARG(QString, ""));

        Q_ASSERT(!runRet.contains(STR_KEY_ERROR));
        return;
    }

    return;
}

void SessionChannel::retry(const QString &params)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "sendMessage error: " << err.errorString();
        return;
    }

    auto root = doc.object();
    QString sessionId = root[STR_KEY_SESSION_ID].toString();
    QString conversationId = root[STR_KEY_CONVERSATION_ID].toString();
    if (sessionId.isEmpty() || conversationId.isEmpty()) {
        qCWarning(logAIGUI) << "sendMessage error: sessionId or conversationId is empty";
        return;
    }

    QString assist = root.value(STR_KEY_ASSISTANT).toString();
    QString model = root.value(STR_KEY_MODEL).toString();
    QString modelName = root.value(STR_KEY_MODEL_NAME).toString();

    // 处理聊天记录
    {
        auto msgs = root[STR_KEY_MESSAGE].toObject();
        QString pre = msgs.value(STR_KEY_PREVIOUS).toString();
        QString currentMsgId = msgs.value(STR_KEY_ID).toString();

        ConversationRecordPtr record = ConvMgr()->getConversation(conversationId);
        if (record) {
            qCDebug(logAIGUI) << "retry conversation" << conversationId;
        } else {
            qCWarning(logAIGUI) << "can not found conversation" << conversationId;
            return;
        }

        MessageNodePtr msgNode(record->messageAt(currentMsgId));
        if (msgNode) {
            qCDebug(logAIGUI) << "retry question" << currentMsgId;
            msgNode->setModelName(modelName);
            msgNode->setModelId(model);
            const QVariantHash extension = msgs.value(STR_KEY_EXTENSION).toObject().toVariantHash();
            if (!extension.isEmpty())
                msgNode->setExtension(extension);
        } else {
            qCWarning(logAIGUI) << "can not found question" << currentMsgId;
            return;
        }

        record->setCurrentMessage(currentMsgId);
    }

    // 创建会话
    {
        auto obj = WebSessMgr()->createSession(assist, sessionId);
        if (obj.value(STR_KEY_ID).toString() != sessionId) {
            qCWarning(logAIGUI) << "createSession error: " << obj.value(STR_KEY_MESSAGE).toString();
            QMetaObject::invokeMethod(this, "sessionEvent", Qt::QueuedConnection,
                Q_ARG(int , SessionEvent::SeError), Q_ARG(QString, sessionId), Q_ARG(QString, ""));
            return;
        }
    }

    auto paramsObj = root.value(STR_KEY_PARAMS).toObject();
    paramsObj[STR_KEY_RETRY] = true;
    auto runRet = WebSessMgr()->runSession(sessionId, model, ConvMgr()->getConversation(conversationId), paramsObj.toVariantHash());
    if (runRet.contains(STR_KEY_ERROR)) {
        qCWarning(logAIGUI) << "runSession error: " << runRet.value(STR_KEY_MESSAGE).toString();
        QMetaObject::invokeMethod(this, "sessionEvent", Qt::QueuedConnection,
            Q_ARG(int , SessionEvent::SeError), Q_ARG(QString, sessionId), Q_ARG(QString, ""));
        return;
    }

    return;
}

void SessionChannel::cancel(const QString &params)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "cancle error: " << err.errorString();
        return;
    }

    auto root = doc.object();
    QString sessionId = root[STR_KEY_SESSION_ID].toString();
    if (sessionId.isEmpty()) {
        qCWarning(logAIGUI) << "cancle error: sessionId is empty";
        return;
    }

    WebSessMgr()->cancelSession(sessionId);
}

void SessionChannel::invokeAction(const QString &id, const QString &json)
{
    auto session = WebSessMgr()->getSession(id);
    if (session.isNull()) {
        qCWarning(logAIGUI) << "invokeAction error: session not found" << id;
        return;
    }

    auto assit = session->assistant();
    if (assit.isNull()) {
        qCWarning(logAIGUI) << "invokeAction error: assistant not found" << id;
        return;
    }

    QJsonObject aciton;
    {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            qCWarning(logAIGUI) << "invokeAction error: " << err.errorString();
            return;
        }

        aciton = doc.object();
    }

    assit->invokeAction(aciton);
    return;
}
