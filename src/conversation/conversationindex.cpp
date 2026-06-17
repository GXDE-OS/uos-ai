#include "conversationindex.h"
#include "global_key_define.h"
#include "assistantmanager.h"

#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logConv)

using namespace uos_ai;

ConversationIndexItem ConversationIndexItem::fromRecord(const ConversationRecordPtr &record)
{
    ConversationIndexItem item;
    item.id = record->id();
    item.title = record->title();
    item.updateTime = record->updateTime().toMSecsSinceEpoch();
    item.assistant = record->assistantId();
    item.introduction = record->generateSummary();
    return item;
}

QJsonObject ConversationIndexItem::toJson() const
{
    QJsonObject obj;
    obj[STR_KEY_ID] = id;
    obj[STR_KEY_TITLE] = title;
    obj[STR_KEY_UPDATED_AT] = updateTime;
    obj[STR_KEY_ASSISTANT] = assistant;
    obj[STR_KEY_ASSISTANT_NAME] = assistantName;
    obj[STR_KEY_INTRODUCTION] = introduction;
    return obj;
}

bool ConversationIndexItem::fromJson(const QJsonObject &json)
{
    if (json.contains(STR_KEY_ID) && json.contains(STR_KEY_TITLE) && json.contains(STR_KEY_UPDATED_AT)) {
        id = json[STR_KEY_ID].toString();
        title = json[STR_KEY_TITLE].toString();
        updateTime = json[STR_KEY_UPDATED_AT].toVariant().toLongLong();
        assistant = json[STR_KEY_ASSISTANT].toString();
        assistantName = json[STR_KEY_ASSISTANT_NAME].toString();
        introduction = json[STR_KEY_INTRODUCTION].toString();
        return true;
    }
    return false;
}

ConversationIndex::ConversationIndex(const QString &path):
    m_storagePath(path)
{

}

ConversationIndex::~ConversationIndex()
{

}

bool ConversationIndex::addOrUpdateIndex(const ConversationRecordPtr &record)
{
    if (!record) {
        qCWarning(logConv) << "Invalid conversation record";
        return false;
    }
    
    ConversationIndexItem item;
    item.id = record->id();
    item.title = record->title();
    item.updateTime = record->updateTime().toMSecsSinceEpoch();
    item.assistant = record->assistantId();
    
    // Match assistantName from AssistantManager using assistantId
    QList<AssistantInfo> assistantList = AssistantMgr->getAssistantList();
    for (const auto &info : assistantList) {
        if (info.id == item.assistant) {
            item.assistantName = info.name;
            break;
        }
    }
    
    item.introduction = record->generateSummary();
    
    m_indexes[item.id] = item;

    // 保存到文件
    return save();
}

bool ConversationIndex::removeIndex(const QString &id)
{
    if (m_indexes.remove(id) > 0) {
        return save();
    }
    return false;
}

void ConversationIndex::clearIndex()
{
    m_indexes.clear();
    save();
}

QVector<ConversationIndexItem> ConversationIndex::getAllIndexes() const
{
    QVector<ConversationIndexItem> items;
    for (const auto &item : m_indexes) {
        items.append(item);
    }

    // 按更新时间降序排序（最新的在前面）
    std::sort(items.begin(), items.end(), [](const ConversationIndexItem &a, const ConversationIndexItem &b) {
        return a.updateTime > b.updateTime;
    });

    return items;
}

ConversationIndexItem ConversationIndex::getIndex(const QString &id) const
{
    return m_indexes.value(id);
}

int ConversationIndex::count() const
{
    return m_indexes.count();
}

bool ConversationIndex::contains(const QString &id) const
{
    return m_indexes.contains(id);
}

bool ConversationIndex::save()
{
    // 确保存储目录存在
    QDir dir(m_storagePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QJsonArray indexArray;
    for (const auto &item : m_indexes) {
        indexArray.append(item.toJson());
    }
    
    QJsonObject rootObj;
    rootObj["version"] = "1.0";
    rootObj["indexes"] = indexArray;
    
    QFile file(getIndexFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(logConv) << "Failed to open index file for writing:" << getIndexFilePath();
        return false;
    }
    
    file.write(QJsonDocument(rootObj).toJson());
    file.close();
    
    qCDebug(logConv) << "Conversation index saved, total items:" << m_indexes.size();
    
    // 触发保存回调
    if (m_saveCallback) {
        m_saveCallback();
    }
    
    return true;
}

void ConversationIndex::setSaveCallback(const IndexSaveCallback &callback)
{
    m_saveCallback = callback;
}

bool ConversationIndex::load()
{
    QFile file(getIndexFilePath());
    if (!file.exists()) {
        qCDebug(logConv) << "Index file does not exist, will create new one";
        return true;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(logConv) << "Failed to open index file for reading:" << getIndexFilePath();
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    
    if (error.error != QJsonParseError::NoError) {
        qCWarning(logConv) << "Failed to parse index file:" << error.errorString();
        return false;
    }
    
    QJsonObject rootObj = doc.object();
    if (!rootObj.contains("indexes") || !rootObj["indexes"].isArray()) {
        qCWarning(logConv) << "Invalid index file format";
        return false;
    }
    
    m_indexes.clear();
    QJsonArray indexArray = rootObj["indexes"].toArray();
    for (const auto &itemValue : indexArray) {
        if (itemValue.isObject()) {
            ConversationIndexItem item;
            if (item.fromJson(itemValue.toObject())) {
                m_indexes[item.id] = item;
            }
        }
    }

    qCDebug(logConv) << "Conversation index loaded, total items:" << m_indexes.size();
    return true;
}

QString ConversationIndex::getIndexFilePath() const
{
    return m_storagePath + "/index.json";
}
