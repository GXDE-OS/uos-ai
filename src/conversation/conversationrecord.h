#ifndef CONVERSATIONRECORD_H
#define CONVERSATIONRECORD_H

#include "global_define.h"
#include "messagenode.h"

#include <QObject>
#include <QDateTime>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QReadWriteLock>

namespace uos_ai {

// 对话记录类
class ConversationRecord : public QObject
{
    Q_OBJECT
public:
    explicit ConversationRecord();
    explicit ConversationRecord(const QString &id);
    ~ConversationRecord();
    
    void setAssistantId(const QString &assistantId);
    QString assistantId() const;

    void setModelId(const QString &modelId);
    QString modelId() const;

    void setCurrentMessage(const QString &msgId);
    QString currentMessage() const;

    // 基本属性
    QString id() const;
    QString title() const;
    void setTitle(const QString &title);
    QDateTime createTime() const;
    QDateTime updateTime() const;
    QString introduction() const;
    void setIntroduction(const QString &introduction);

    bool addMessage(const QString &pre, const MessageNodePtr &message);
    QList<MessageNodePtr> messageAt(const QStringList &ids) const;
    MessageNodePtr messageAt(const QString &id) const;

    bool switchNext(const QString &target, const QString &next);

    // 持久化
    QJsonObject toJson() const;
    bool saveToFile(const QString &filePath) const;
    static QSharedPointer<ConversationRecord> fromJson(const QJsonObject &json);
    static QSharedPointer<ConversationRecord> loadFromFile(const QString &filePath);
    
    // 工具方法
    QString generateSummary() const;
    bool isEmpty() const;
    
    QList<MessageNodePtr> history(const QString &leafId) const;

    void updateTimestamp();
signals:
    void updateTimeChanged(const QDateTime &updateTime);
    void messageAdded(int index);

private:
    QString m_title;
    QString m_assistantId;
    QString m_modelId;
    QString m_introduction;
    QDateTime m_createTime;
    QDateTime m_updateTime;
    QString m_currentMsg;
    MessageNodePtr m_root = nullptr;
    QMap<QString, MessageNodePtr> m_messages;
    mutable QReadWriteLock m_lock;
};

using ConversationRecordPtr = QSharedPointer<ConversationRecord>;
}

#endif // CONVERSATIONRECORD_H
