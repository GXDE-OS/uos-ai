#ifndef CONVERSATIONINDEX_H
#define CONVERSATIONINDEX_H

#include "global_define.h"
#include "conversationrecord.h"

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <functional>

namespace uos_ai {

// 索引保存回调函数类型
using IndexSaveCallback = std::function<void()>;

// 对话记录索引项
struct ConversationIndexItem {
    QString id;
    QString title;
    qint64 updateTime;
    QString assistant;
    QString assistantName;
    QString introduction;
    
    ConversationIndexItem() = default;
    ConversationIndexItem(const QString &id, const QString &title, qint64 updateTime, const QString &assistant, const QString &assistantName, const QString &introduction);
    
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &json);
};

// 对话索引管理类
class ConversationIndex
{
public:
    explicit ConversationIndex(const QString &path);
    ~ConversationIndex();

    // 索引管理
    bool addOrUpdateIndex(const ConversationRecordPtr &record);
    bool removeIndex(const QString &id);
    void clearIndex();
    
    // 索引查询
    QVector<ConversationIndexItem> getAllIndexes() const;
    ConversationIndexItem getIndex(const QString &id) const;
    int count() const;
    bool contains(const QString &id) const;
    
    // 持久化
    bool save();
    bool load();
    
    // 回调设置
    void setSaveCallback(const IndexSaveCallback &callback);

private:
    QString getIndexFilePath() const;

private:
    QMap<QString, ConversationIndexItem> m_indexes;
    QString m_storagePath;
    IndexSaveCallback m_saveCallback;
};

}

#endif // CONVERSATIONINDEX_H
