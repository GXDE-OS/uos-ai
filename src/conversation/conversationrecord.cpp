#include "global_key_define.h"
#include "conversationrecord.h"

#include <QUuid>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

using namespace uos_ai;

ConversationRecord::ConversationRecord()
    : QObject(nullptr)
    , m_title(tr("New Conversation"))
    , m_createTime(QDateTime::currentDateTime())
    , m_updateTime(m_createTime)
    , m_root(MessageNodePtr(new MessageNode))
{
    m_root->setRole(MrVirtual);
}

ConversationRecord::ConversationRecord(const QString &id)
    : QObject(nullptr)
    , m_title(tr("New Conversation"))
    , m_createTime(QDateTime::currentDateTime())
    , m_updateTime(m_createTime)
    , m_root(MessageNodePtr(new MessageNode))
{
    m_root->setRole(MrVirtual);
    m_root->setId(id);
}

ConversationRecord::~ConversationRecord()
{
}

void ConversationRecord::setAssistantId(const QString &assistantId)
{
    QWriteLocker locker(&m_lock);
    m_assistantId = assistantId;
}

QString ConversationRecord::assistantId() const
{
    QReadLocker locker(&m_lock);
    return m_assistantId;
}

void ConversationRecord::setModelId(const QString &modelId)
{
    QWriteLocker locker(&m_lock);
    m_modelId = modelId;
}

QString ConversationRecord::modelId() const
{
    QReadLocker locker(&m_lock);
    return m_modelId;
}

void ConversationRecord::setCurrentMessage(const QString &msgId)
{
    QWriteLocker locker(&m_lock);
    m_currentMsg = msgId;
}

QString ConversationRecord::currentMessage() const
{
    QReadLocker locker(&m_lock);
    return m_currentMsg;
}

QString ConversationRecord::id() const
{
    QReadLocker locker(&m_lock);
    return m_root->getId();
}

QString ConversationRecord::title() const
{
    QReadLocker locker(&m_lock);
    return m_title;
}

void ConversationRecord::setTitle(const QString &title)
{
    QWriteLocker locker(&m_lock);
    if (m_title != title) {
        m_title = title;
    }
}

QDateTime ConversationRecord::createTime() const
{
    QReadLocker locker(&m_lock);
    return m_createTime;
}

QDateTime ConversationRecord::updateTime() const
{
    QReadLocker locker(&m_lock);
    return m_updateTime;
}

QString ConversationRecord::introduction() const
{
    QReadLocker locker(&m_lock);
    return m_introduction;
}

void ConversationRecord::setIntroduction(const QString &introduction)
{
    QWriteLocker locker(&m_lock);
    if (m_introduction != introduction) {
        m_introduction = introduction;
    }
}

bool ConversationRecord::addMessage(const QString &pre, const MessageNodePtr &message)
{
    if (message.isNull())
        return false;
    
    QWriteLocker locker(&m_lock);
    
    if (pre.isEmpty()) {
        message->setPrevious(m_root->getId());
        m_root->appendNext(message->getId());
        if (m_root->getCurNext().isEmpty())
            m_root->setCurNext(message->getId());
    } else {
        auto pNode = m_messages.value(pre);
        if (pNode.isNull())
            return false;

        pNode->appendNext(message->getId());
        message->setPrevious(pre);
        if (pNode->getCurNext().isEmpty())
            pNode->setCurNext(message->getId());
    }

    m_messages.insert(message->getId(), message);
    locker.unlock();

    updateTimestamp();
    return true;
}

QList<MessageNodePtr> ConversationRecord::messageAt(const QStringList &ids) const
{
    QReadLocker locker(&m_lock);
    QList<MessageNodePtr> nodes;
    for (const QString &id : ids) {
        if (id == m_root->getId()) {
            nodes.append(m_root);
            continue;
        }
        auto node = m_messages.value(id);
        nodes.append(node);
    }

    return nodes;
}

MessageNodePtr ConversationRecord::messageAt(const QString &id) const
{
    QReadLocker locker(&m_lock);
    return m_messages.value(id);
}

bool ConversationRecord::switchNext(const QString &target, const QString &next)
{
    if (target.isEmpty() || next.isEmpty())
        return false;
    
    QWriteLocker locker(&m_lock);
    auto node = m_messages.value(target);
    if (node.isNull())
        return false;
    
    if (!node->getNext().contains(next))
        return false;

    node->setCurNext(next);
    return true;
}

QJsonObject ConversationRecord::toJson() const
{
    QReadLocker locker(&m_lock);
    QJsonObject json;

    QJsonObject rootJson;
    rootJson[STR_KEY_ID] = m_root->getId();
    rootJson[STR_KEY_NEXT] = QJsonArray::fromStringList(m_root->getNext());
    rootJson[STR_KEY_CUR_NEXT] = m_root->getCurNext();
    rootJson[STR_KEY_ASSISTANT] = m_assistantId;
    rootJson[STR_KEY_MODEL] = m_modelId;
    rootJson[STR_KEY_TITLE] = m_title;
    rootJson[STR_KEY_INTRODUCTION] = m_introduction;
    json[STR_KEY_ROOT] = rootJson;

    QJsonObject messagesJson;
    for (const auto &message : m_messages) {
        QJsonObject messageJson = message->toJson();
        messagesJson[message->getId()] = messageJson;
    }

    json[STR_KEY_MESSAGES] = messagesJson;

    return json;
}

QSharedPointer<ConversationRecord> ConversationRecord::fromJson(const QJsonObject &json)
{
    if (json.isEmpty())
        return QSharedPointer<ConversationRecord>();

    ConversationRecordPtr record(new ConversationRecord);

    // Parse root
    QJsonObject rootJson = json.value(STR_KEY_ROOT).toObject();
    QString rootId = rootJson.value(STR_KEY_ID).toString();
    if (!rootId.isEmpty()) {
        record->m_root->setId(rootId);
    }
    record->m_root->setRole(MrVirtual);

    QStringList nextList;
    QJsonArray nextArray = rootJson.value(STR_KEY_NEXT).toArray();
    for (const QJsonValue &val : nextArray) {
        nextList.append(val.toString());
    }
    record->m_root->setNext(nextList);

    QString curNext = rootJson.value(STR_KEY_CUR_NEXT).toString();
    record->m_root->setCurNext(curNext);

    QString assistantId = rootJson.value(STR_KEY_ASSISTANT).toString();
    record->m_assistantId = assistantId;

    QString modelId = rootJson.value(STR_KEY_MODEL).toString();
    record->m_modelId = modelId;

    QString title = rootJson.value(STR_KEY_TITLE).toString();
    record->m_title = title;

    QString introduction = rootJson.value(STR_KEY_INTRODUCTION).toString();
    record->m_introduction = introduction;

    // Parse messages
    QJsonObject messagesJson = json.value(STR_KEY_MESSAGES).toObject();
    for (auto it = messagesJson.constBegin(); it != messagesJson.constEnd(); ++it) {
        QString msgId = it.key();
        QJsonObject msgJson = it.value().toObject();

        MessageNodePtr msgNode = MessageNode::fromJson(msgJson);
        msgNode->setId(msgId);
        if (!msgNode.isNull()) {
            record->m_messages.insert(msgId, msgNode);
        }
    }

    return record;
}

bool ConversationRecord::saveToFile(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    QJsonDocument doc(toJson());
    file.write(doc.toJson());
    file.close();
    
    return true;
}

QSharedPointer<ConversationRecord> ConversationRecord::loadFromFile(const QString &filePath)
{
    ConversationRecordPtr ptr;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for reading:" << filePath;
        return ptr;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (doc.isNull()) {
        qWarning() << "Invalid JSON document:" << filePath;
        return ptr;
    }
    
    ptr = fromJson(doc.object());
    return ptr;
}

QString ConversationRecord::generateSummary() const
{
    QReadLocker locker(&m_lock);
    return m_introduction.isEmpty() ? tr("Null") : m_introduction.trimmed();
}

QList<MessageNodePtr> ConversationRecord::history(const QString &leafId) const
{
    QReadLocker locker(&m_lock);
    QList<MessageNodePtr> result;
    QString currentId = leafId;
    
    while (!currentId.isEmpty()) {
        if (currentId == m_root->getId())
             break;

        auto node = m_messages.value(currentId);
        if (node.isNull())
            break;

        result.prepend(node);
        currentId = node->getPrevious();
    }
    
    return result;
}

bool ConversationRecord::isEmpty() const
{
    QReadLocker locker(&m_lock);
    return m_messages.isEmpty();
}

void ConversationRecord::updateTimestamp()
{
    QWriteLocker locker(&m_lock);
    m_updateTime = QDateTime::currentDateTime();
    locker.unlock();

    emit updateTimeChanged(m_updateTime);
}
