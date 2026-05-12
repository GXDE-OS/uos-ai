#ifndef CONVERSATIONMANAGER_H
#define CONVERSATIONMANAGER_H

#include "global_define.h"
#include "conversationrecord.h"
#include "conversationindex.h"

#include <QObject>
#include <QMap>
#include <QString>
#include <QVector>
#include <QReadWriteLock>

namespace uos_ai {

class ConversationManager : public QObject
{
    Q_OBJECT
public:
    static ConversationManager *instance();

    void refresh();

    // 对话管理
    ConversationRecordPtr createConversation(QString id = "");
    ConversationRecordPtr getConversation(const QString &id);
    void deleteConversation(const QStringList &ids);
    
    void clearAllConversations();

    void releaseConversation(const QStringList &ids);
    
    // 属性访问
    int conversationCount() const;
    QVector<ConversationIndexItem> conversationIndexes() const;

    // 路径读取
    QString getConversationFilePath(const QString &id) const;

    // 持久化
    bool saveConversation(const QString &id);
    bool loadConversation(const QString &id);
    bool addOrUpdateIndex(const ConversationRecordPtr &record);

signals:
    void indexChanged();
    void changeToConversation(const QString &assistantId, const QString &conversationId);

private:
    explicit ConversationManager(QObject *parent = nullptr);
    ~ConversationManager();

private:
    QMap<QString, ConversationRecordPtr> m_conversations;
    mutable QReadWriteLock m_conversationsLock;
    QScopedPointer<ConversationIndex> m_index;
    QString m_storagePath;
};

}

#define ConvMgr ConversationManager::instance

#endif // CONVERSATIONMANAGER_H
