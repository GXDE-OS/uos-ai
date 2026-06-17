#ifndef CONVERSATIONSEARCH_H
#define CONVERSATIONSEARCH_H

#include "global_define.h"
#include "conversationindex.h"

#include <QObject>
#include <QThread>
#include <QReadWriteLock>
#include <QString>
#include <QStringList>
#include <QVector>

namespace uos_ai {

// Q_ARG 是预处理器宏，类型参数中不能包含逗号，否则会被误解析为多个宏参数
typedef QMap<QString, QStringList> SearchIndexMap;

class ConversationSearchWorker : public QObject
{
    Q_OBJECT
public:
    explicit ConversationSearchWorker(const QString &storagePath, const QString &indexFileName,
                                      QObject *parent = nullptr);

public slots:
    void doInit();
    void doUpdateIndex(const ConversationRecordPtr &record);
    void doSearch(const SearchIndexMap &indexes, const QString &keyword, int resultLength);

signals:
    // 搜索索引加载完成信号，key为会话id，value为切片后的文本内容列表
    void finished(const SearchIndexMap &indexes);
    void updateFinished(const QString &id, const QStringList &contents);
    void searchFinished(const SearchIndexMap &results);

private:
    QString m_storagePath;
    QString m_indexFileName;
};

class ConversationSearch : public QObject
{
    Q_OBJECT
public:
    static ConversationSearch *instance();

    void initSearchIndexes();
    // 获取指定会话的搜索索引内容（切片后的文本列表）
    QStringList getSearchIndex(const QString &id) const;
    // 更新指定会话的搜索索引（异步，在工作线程中执行）
    void updateSearchIndex(const ConversationRecordPtr &record);
    // 删除指定会话的搜索索引
    void removeSearchIndex(const QString &id);
    // 按关键词搜索（异步，在工作线程中执行）
    void searchByKeyword(const QString &keyword);

    // 历史显示索引管理（搜索结果展示用）
    QVector<ConversationIndexItem> getAllHistoryIndexes() const;
    void addOrUpdateHistoryIndex(const ConversationIndexItem &item);
    void removeHistoryIndex(const QString &id);
    void clearHistoryIndexes();
    void resetHistoryIndexes(const QMap<QString, ConversationIndexItem> &items);

signals:
    void indexSearchChanged();  // 搜索范围变化
    void indexContentLoaded(const SearchIndexMap &indexes, int maxLength);
private:
    explicit ConversationSearch(QObject *parent = nullptr);
    ~ConversationSearch();

private slots:
    void onInitFinished(const SearchIndexMap &indexes);
    void onUpdateFinished(const QString &id, const QStringList &contents);
    void onSearchFinished(const SearchIndexMap &results);

private:
    // 搜索索引表，key为会话id，value为切片后的文本内容列表
    SearchIndexMap m_indexesSearch;
    QMap<QString, ConversationIndexItem> m_historyIndexes;
    int m_searchResultLength = 500;    // 搜索结果最大长度
    mutable QReadWriteLock m_lock;
    mutable QReadWriteLock m_historyLock;
    QString m_storagePath;
    QString m_indexFileName;
    QThread m_initThread;
    QThread m_updateThread;
    ConversationSearchWorker *m_updateWorker = nullptr;
};

}

#define ConvSearch ConversationSearch::instance

#endif // CONVERSATIONSEARCH_H
