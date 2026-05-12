#include "appdbusobject.h"
#include "private/welcomedialog.h"
#include "model/modelvendor.h"
#include "database/appdatabase.h"
#include "database/usedmodeltable.h"
#include "assistant/assistantmanager.h"
#include "app/application.h"
#include "session/sessionmanager.h"
#include "global_define.h"
#include "conversation/conversationrecord.h"
#include "util.h"

#include <QtDBus>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

AppDbusObject::AppDbusObject(const QString &appId)
    : QObject(nullptr)
    , app(appId)
{

    chatSession = SessionManager::instance(app);
    connect(chatSession.data(), &SessionManager::sessionEvent, this, &AppDbusObject::onEvent, Qt::QueuedConnection);
    connect(ModelVendor::instance(), &ModelVendor::modelChanged, this, [this]() {
        if (!WelcomeDialog::isAgreed())
            return;
        emit llmAccountLstChanged(currentLLMAccountId(), queryLLMAccountList());
    }, Qt::QueuedConnection);
}

AppDbusObject::~AppDbusObject()
{
    SessionManager::destroyInstance(app);
}

void AppDbusObject::cancelRequestTask(const QString &id)
{
    if (!checkAgreement())
        return;

    qCDebug(logDBus) << "Cancelling request task with id:" << id;

    chatSession->cancelSession(id);
    chatTask.remove(id);
}

void AppDbusObject::executionAborted()
{
    if (!checkAgreement())
        return;

    qCWarning(logDBus) << "Execution aborted";
    for (auto id : chatSession->getAllSessionIds()) {
        chatSession->cancelSession(id);
        chatSession->removeSession(id);
    }

    chatTask.clear();
}

QStringList AppDbusObject::requestChatText(const QString &llmId, const QString &conversation, qreal temperature, bool stream)
{
    if (!checkAgreement())
        return QStringList();

    uint pid = QDBusConnection::sessionBus().interface()->servicePid(message().service());
    if (app != Util::queryProcessName(pid)) {
        qCWarning(logDBus) << "Access denied for appId:" << Util::queryProcessName(pid);
        sendErrorReply(QDBusError::AccessDenied, "no permission to access!");
            return QStringList();
    }

    if (llmId.isEmpty() || conversation.isEmpty()) {
        qCWarning(logDBus) << "Empty llmId or conversation, skipping request";
        sendErrorReply(QDBusError::InvalidArgs, "Empty llmId or conversation");
        return QStringList();
    }

    qCDebug(logDBus) << "Requesting chat text with llmId:" << llmId << "temperature:" << temperature << "stream:" << stream;
    QVariantHash params = {
        {STR_KEY_STREAM, QVariant(stream)},
        {STR_KEY_THINKING, QVariant(false)},
    };

    QString sessionId = GlobalUtil::generateMsId();

    // 创建会话
    {
        auto obj = chatSession->createSession(STR_KEY_UOS_AI_CHAT, sessionId);
        if (obj.value(STR_KEY_ID).toString() != sessionId) {
            qCWarning(logDBus) << "createSession error: " << llmId << conversation;
            sendErrorReply(QDBusError::InvalidArgs);
            return QStringList();
        }
    }
    auto record = convertRequestion(sessionId, conversation);
    auto runRet = chatSession->runSession(sessionId, llmId, record, params);
    if (runRet.contains(STR_KEY_ERROR)) {
        qCWarning(logDBus) << "runSession error: " << runRet.value(STR_KEY_MESSAGE).toString();
        sendErrorReply(QDBusError::InvalidArgs, runRet.value(STR_KEY_MESSAGE).toString());
        return QStringList();
    }

    ChatTask task;
    task.id = sessionId;
    task.stream = stream;
    task.record = record;
    chatTask.insert(sessionId, task);

    return QStringList{sessionId, "chatTextReceived"};
}

bool AppDbusObject::setCurrentLLMAccountId(const QString &id)
{
    if (!checkAgreement())
        return false;

    qCDebug(logDBus) << "Setting current LLM account ID:" << id;
    auto *db = AppDatabase::instance();
    UsedModelTable usedModel = UsedModelTable::getByAssistant(db, app);

    if (!usedModel.assistant().isEmpty()) {
        // Update existing record
        usedModel.setModel(id);
        return usedModel.update(db);
    } else {
        // Create new record
        UsedModelTable newRecord = UsedModelTable::create(app, id, "");
        return newRecord.save(db);
    }
}

QString AppDbusObject::currentLLMAccountId()
{
    if (!checkAgreement())
        return QString();


    auto *db = AppDatabase::instance();
    UsedModelTable usedModel = UsedModelTable::getByAssistant(db, app);

    if (!usedModel.assistant().isEmpty()) {
        return usedModel.model();
    }

    usedModel = UsedModelTable::getByAssistant(db, STR_KEY_UOS_AI_GENERIC);
    if (!usedModel.assistant().isEmpty()) {
        return usedModel.model();
    }

    auto models = AssistantMgr->availableModels(STR_KEY_UOS_AI_CHAT);
    if (!models.isEmpty()) {
        return models.first()->id;
    }
    
    // 模型列表为空，返回空字符串
    return "";
}

QString AppDbusObject::queryLLMAccountList()
{
    if (!checkAgreement())
        return QString();

    QJsonArray llmAccountArray;
    auto models = AssistantManager::instance()->availableModels(STR_KEY_UOS_AI_CHAT);
    for (auto model : models) {
        QJsonObject accountObj;
        accountObj.insert("id", model->id);
        accountObj["displayname"] = model->model.name;
        accountObj["llmname"] = model->model.modelId;
        accountObj["type"] = ModelVendor::isUosProvider(model) ? 1 : 0;
        accountObj["icon"] = model->model.icon;
        llmAccountArray.append(accountObj);
    }

    return QJsonDocument(llmAccountArray).toJson(QJsonDocument::Compact);
}

void AppDbusObject::launchLLMUiPage(bool showAddllmPage)
{
    qCDebug(logDBus) << "Launching LLM UI page, showAddllmPage:" << showAddllmPage;
    QMetaObject::invokeMethod(aiApp, "showConfig", Qt::QueuedConnection, Q_ARG(int, MgmtWindow::Page::ModelList));
}

void AppDbusObject::onEvent(int event, const QString &id, const QString &json)
{
    if (!chatTask.contains(id))
        return;

    if (event == SessionEvent::SeError) {
        emit error(id, QNetworkReply::InternalServerError, json);
    } else if (event == SessionEvent::SeMessage) {
        if (!chatTask[id].stream)
            return;

        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        RenderMessage msg = RenderMessage::fromJson(doc.object());
        if (msg.type == CntText) {
            emit chatTextReceived(id, msg.data.toString());
        }
    } else if (event == SessionEvent::SeFinished) {
        auto task = chatTask.take(id);
        if (task.stream)
            return;

        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QString msgId = doc.object().value(STR_KEY_ID).toString();
        if (task.record.isNull()) {
            qCWarning(logDBus) << "No record found for session:" << id;
            return;
        }

        auto msgNode = task.record->messageAt(msgId);
        if (msgNode.isNull()) {
            qCWarning(logDBus) << "No message found for ID:" << msgId;
            return;
        }

        auto list = msgNode->getMessage();
        if (list.isEmpty()) {
            qCWarning(logDBus) << "No message content found for ID:" << msgId;
            return;
        }
        QString content;
        for (const MetaMessage &mm : list.last().content) {
            if (mm.type == CntText)
                content.append(mm.data.toString());
        }

        emit chatTextReceived(id, content);
    }
}

bool AppDbusObject::checkAgreement()
{
    if (WelcomeDialog::isAgreed())
        return true;

    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(logDBus) << "Request not in main thread, cannot show WelcomeDialog";
        return false;
    }

    qCDebug(logDBus) << "Showing WelcomeDialog";
    WelcomeDialog::instance(false)->exec();
    return WelcomeDialog::isAgreed();
}

QSharedPointer<ConversationRecord> AppDbusObject::convertRequestion(const QString &id, const QString &json)
{
    QSharedPointer<ConversationRecord> record(new ConversationRecord(id));
    MessageNodePtr msgNode(new MessageNode);
    msgNode->setId(GlobalUtil::generateMsId());
    msgNode->setRole(MrUser);

    record->addMessage("", msgNode);
    record->setCurrentMessage(msgNode->getId());

    const QJsonDocument &document = QJsonDocument::fromJson(json.toUtf8());
    if (document.isArray()) {
        for ( QJsonValue val : document.array()) {
            if (!val.isObject())
                continue;

            ModelMessage modelMsg;
            modelMsg.role = val.toObject().value(STR_KEY_ROLE).toString();
            MetaMessage metaMsg;
            metaMsg.type = CntText;
            metaMsg.data = val.toObject().value(STR_KEY_CONTENT).toString();
            modelMsg.content.append(metaMsg);
            msgNode->appendMessage(modelMsg);
        }
    } else {
        ModelMessage modelMsg;
        modelMsg.role = STR_KEY_USER;
        MetaMessage metaMsg;
        metaMsg.type = CntText;
        metaMsg.data = json;
        modelMsg.content.append(metaMsg);
        msgNode->appendMessage(modelMsg);
    }

    return record;
}
