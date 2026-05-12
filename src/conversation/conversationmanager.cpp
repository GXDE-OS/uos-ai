#include "conversationmanager.h"

#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logConv)

using namespace uos_ai;

ConversationManager *ConversationManager::instance()
{
    static ConversationManager ins;
    return &ins;
}

void ConversationManager::refresh()
{
    QWriteLocker locker(&m_conversationsLock);
    m_conversations.clear();
    m_index->load();
}

ConversationManager::ConversationManager(QObject *parent)
    : QObject(parent)
    , m_storagePath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/conversations")
{
    // 确保存储目录存在
    QDir dir(m_storagePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    m_index.reset(new ConversationIndex(m_storagePath));
    
    // 设置索引保存回调
    m_index->setSaveCallback([this]() {
        qCDebug(logConv) << "Conversation index saved, emitting changed signal";
        emit indexChanged();
    });
    
    refresh();
}

ConversationManager::~ConversationManager()
{

}

void ConversationManager::releaseConversation(const QStringList &ids)
{
    for (const QString &id : ids) {
        saveConversation(id);
    }

    QWriteLocker locker(&m_conversationsLock);
    for (const QString &id : ids) {
        m_conversations.take(id);
    }
}

ConversationRecordPtr ConversationManager::createConversation(QString id)
{
    if (id.isEmpty())
        id = GlobalUtil::generateMsId();

    ConversationRecordPtr record(new ConversationRecord(id));
    {
        QWriteLocker locker(&m_conversationsLock);
        m_conversations[id] = record;
    }

    return record;
}

ConversationRecordPtr ConversationManager::getConversation(const QString &id)
{
    {
        QReadLocker locker(&m_conversationsLock);
        if (m_conversations.contains(id)) {
            return m_conversations[id];
        }
    }
    
    // 不在锁内调用loadConversation避免死锁
    qCDebug(logConv) << "load conversation with id" << id;
    loadConversation(id);
    
    {
        QReadLocker locker(&m_conversationsLock);
        return m_conversations.value(id, nullptr);
    }
}

void ConversationManager::deleteConversation(const QStringList &ids)
{
    QWriteLocker locker(&m_conversationsLock);
    for (const QString &id : ids) {
        m_conversations.take(id);
    }
    locker.unlock();

    // 删除文件记录
    for (const QString &id : ids) {
        QString filePath = getConversationFilePath(id);
        if (QFile::exists(filePath)) {
            if (!QFile::remove(filePath)) {
                qCWarning(logConv) << "Failed to delete conversation file:" << filePath;
            } else {
                qCDebug(logConv) << "Conversation file deleted:" << filePath;
            }
        }
    }

    // 从索引中删除
    for (const QString &id : ids) {
        m_index->removeIndex(id);
    }
}

void ConversationManager::clearAllConversations()
{
    QWriteLocker locker(&m_conversationsLock);
    m_conversations.clear();
    locker.unlock();

    // 清空所有会话文件
    QDir dir(m_storagePath);
    if (dir.exists()) {
        QStringList filters;
        filters << "*.json";
        dir.setNameFilters(filters);
        
        QStringList files = dir.entryList(QDir::Files);
        for (const QString &file : files) {
            QString filePath = m_storagePath + "/" + file;
            if (!QFile::remove(filePath)) {
                qCWarning(logConv) << "Failed to delete conversation file:" << filePath;
            } else {
                qCDebug(logConv) << "Conversation file deleted:" << filePath;
            }
        }
    }

    // 清空索引
    m_index->clearIndex();
}

int ConversationManager::conversationCount() const
{
    QReadLocker locker(&m_conversationsLock);
    return m_index->count();
}

QVector<ConversationIndexItem> ConversationManager::conversationIndexes() const
{
    QReadLocker locker(&m_conversationsLock);
    return m_index->getAllIndexes();
}

bool ConversationManager::saveConversation(const QString &id)
{
    QReadLocker locker(&m_conversationsLock);
    ConversationRecordPtr record = m_conversations.value(id, nullptr);
    if (!record) {
        qCWarning(logConv) << "Conversation not found with id" << id;
        return false;
    }

    locker.unlock();
    return record->saveToFile(getConversationFilePath(id)) && m_index->addOrUpdateIndex(record);
}

bool ConversationManager::loadConversation(const QString &id)
{
    QString path = getConversationFilePath(id);
    if (!QFile::exists(path))
        return false;

    ConversationRecordPtr record =  ConversationRecord::loadFromFile(path);
    if (record) {
        QWriteLocker locker(&m_conversationsLock);
        // 如果已存在相同ID的对话，先删除
        if (m_conversations.contains(id))
            m_conversations.take(id);

        m_conversations[id] = record;
        return true;
    }
    
    return false;
}

bool ConversationManager::addOrUpdateIndex(const ConversationRecordPtr &record)
{
    return m_index->addOrUpdateIndex(record);
}

QString ConversationManager::getConversationFilePath(const QString &id) const
{
    return m_storagePath + "/" + id + ".json";
}
