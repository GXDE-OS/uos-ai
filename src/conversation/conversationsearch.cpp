#include "conversationsearch.h"
#include "conversationrecord.h"
#include "global_key_define.h"

#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>
#include <QMetaObject>

Q_DECLARE_LOGGING_CATEGORY(logConv)

namespace uos_ai {

// 注册元类型，用于跨线程信号槽传递搜索结果
static const int _convSearchResultMetaTypeId = qRegisterMetaType<SearchIndexMap>("SearchIndexMap");
static const int _stringListMetaTypeId = qRegisterMetaType<QStringList>("QStringList");
static const int _convRecordPtrMetaTypeId = qRegisterMetaType<ConversationRecordPtr>("ConversationRecordPtr");

ConversationSearch *ConversationSearch::instance()
{
    static ConversationSearch ins;
    return &ins;
}

ConversationSearch::ConversationSearch(QObject *parent)
    : QObject(parent)
{
    m_storagePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/conversations";
    m_indexFileName = "index.json";
}

ConversationSearch::~ConversationSearch()
{
    m_initThread.quit();
    m_initThread.wait();
    m_updateThread.quit();
    m_updateThread.wait();
}

void ConversationSearch::initSearchIndexes()
{
    qCDebug(logConv) << "initSearchIndexes: storagePath =" << m_storagePath;
    QDir dir(m_storagePath);
    if (!dir.exists())
        return;

    auto *worker = new ConversationSearchWorker(m_storagePath, m_indexFileName);
    worker->moveToThread(&m_initThread);

    connect(&m_initThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &ConversationSearch::destroyed, &m_initThread, &QThread::quit);
    connect(worker, &ConversationSearchWorker::finished,
            this, &ConversationSearch::onInitFinished);
    // 初始化完成后自动退出初始化线程，避免线程空转浪费资源
    connect(worker, &ConversationSearchWorker::finished, &m_initThread, &QThread::quit);

    m_initThread.start();
    QMetaObject::invokeMethod(worker, "doInit", Qt::QueuedConnection);

    // 初始化更新索引的工作线程
    m_updateWorker = new ConversationSearchWorker(m_storagePath, m_indexFileName);
    m_updateWorker->moveToThread(&m_updateThread);

    connect(&m_updateThread, &QThread::finished, m_updateWorker, &QObject::deleteLater);
    connect(this, &ConversationSearch::destroyed, &m_updateThread, &QThread::quit);
    connect(m_updateWorker, &ConversationSearchWorker::updateFinished,
            this, &ConversationSearch::onUpdateFinished);
    connect(m_updateWorker, &ConversationSearchWorker::searchFinished,
            this, &ConversationSearch::onSearchFinished);

    m_updateThread.start();
}

// 搜索索引初始化完成回调，将工作线程加载的索引写入成员变量
void ConversationSearch::onInitFinished(const SearchIndexMap &indexes)
{
    qCDebug(logConv) << "onInitFinished results count:" << indexes.size();
#ifdef DEBUG_LOG
    for (auto it = indexes.constBegin(); it != indexes.constEnd(); ++it) {
        qCDebug(logConv) << "  matched id:" << it.key() << "snippet:" << it.value();
    }
#endif

    {
        QWriteLocker locker(&m_lock);
        m_indexesSearch = indexes;
    }
    emit indexContentLoaded(indexes, m_searchResultLength);
}


void ConversationSearch::removeSearchIndex(const QString &id)
{
    qCDebug(logConv) << "removeSearchIndex:" << id;
    QWriteLocker locker(&m_lock);
    m_indexesSearch.remove(id);
}

void ConversationSearch::searchByKeyword(const QString &keyword)
{
    qCDebug(logConv) << "searchByKeyword:" << keyword;
    if (!m_updateWorker)
        return;

    SearchIndexMap indexesCopy;
    {
        QReadLocker locker(&m_lock);
        indexesCopy = m_indexesSearch;
    }


    QMetaObject::invokeMethod(m_updateWorker, "doSearch",
                              Qt::QueuedConnection,
                              Q_ARG(SearchIndexMap, indexesCopy),
                              Q_ARG(QString, keyword),
                              Q_ARG(int, m_searchResultLength));
}

void ConversationSearch::onSearchFinished(const SearchIndexMap &results)
{
    qCDebug(logConv) << "onSearchFinished results count:" << results.size();
#ifdef QT_DEBUG
    for (auto it = results.constBegin(); it != results.constEnd(); ++it) {
        qCDebug(logConv) << "  matched id:" << it.key() << "snippet:" << it.value();
    }
#endif
    emit indexContentLoaded(results, m_searchResultLength);
}

// 更新指定会话的搜索索引，将耗时操作放入工作线程执行
// 线程安全说明：ConversationRecord 使用读写锁保护，extractTextContent() 有 QReadLocker，
// 所有修改操作有 QWriteLocker，配合 QSharedPointer 引用计数，无需额外拷贝
void ConversationSearch::updateSearchIndex(const ConversationRecordPtr &record)
{
    if (record.isNull() || !m_updateWorker)
        return;

    QMetaObject::invokeMethod(m_updateWorker, "doUpdateIndex",
                              Qt::QueuedConnection, Q_ARG(ConversationRecordPtr, record));
}

// 更新索引完成回调，将工作线程提取的内容写入索引表
void ConversationSearch::onUpdateFinished(const QString &id, const QStringList &contents)
{
    if (id.isEmpty())
        return;

    qCDebug(logConv) << "onUpdateFinished:" << id << "contents count:" << contents.size();
    QWriteLocker locker(&m_lock);
    m_indexesSearch[id] = contents;

    emit indexSearchChanged();
}

QStringList ConversationSearch::getSearchIndex(const QString &id) const
{
    QReadLocker locker(&m_lock);
    return m_indexesSearch.value(id);
}

QVector<ConversationIndexItem> ConversationSearch::getAllHistoryIndexes() const
{
    QReadLocker locker(&m_historyLock);
    QVector<ConversationIndexItem> items;
    for (const auto &item : m_historyIndexes) {
        items.append(item);
    }
    locker.unlock();

    std::sort(items.begin(), items.end(), [](const ConversationIndexItem &a, const ConversationIndexItem &b) {
        return a.updateTime > b.updateTime;
    });

    return items;
}

void ConversationSearch::addOrUpdateHistoryIndex(const ConversationIndexItem &item)
{
    QWriteLocker locker(&m_historyLock);
    m_historyIndexes[item.id] = item;
}

void ConversationSearch::removeHistoryIndex(const QString &id)
{
    {
        QWriteLocker historyLocker(&m_historyLock);
        m_historyIndexes.remove(id);
    }
    {
        QWriteLocker searchLocker(&m_lock);
        m_indexesSearch.remove(id);
    }
    emit indexSearchChanged();
}

void ConversationSearch::clearHistoryIndexes()
{
    {
        QWriteLocker historyLocker(&m_historyLock);
        m_historyIndexes.clear();
    }
    {
        QWriteLocker searchLocker(&m_lock);
        m_indexesSearch.clear();
    }
    emit indexSearchChanged();
}

void ConversationSearch::resetHistoryIndexes(const QMap<QString, ConversationIndexItem> &items)
{
    {
        QWriteLocker locker(&m_historyLock);
        m_historyIndexes = items;
    }
    emit indexSearchChanged();
}

ConversationSearchWorker::ConversationSearchWorker(const QString &storagePath,
                                                   const QString &indexFileName,
                                                   QObject *parent)
    : QObject(parent)
    , m_storagePath(storagePath)
    , m_indexFileName(indexFileName)
{
}

// 在工作线程中加载所有会话的搜索索引
void ConversationSearchWorker::doInit()
{
    QMap<QString, QStringList> result;

    QDir dir(m_storagePath);
    if (!dir.exists()) {
        qCWarning(logConv) << "doInit: storage dir not exists:" << m_storagePath;
        emit finished(result);
        return;
    }

    QFile indexFile(dir.absoluteFilePath(m_indexFileName));
    if (!indexFile.exists() || !indexFile.open(QIODevice::ReadOnly)) {
        qCWarning(logConv) << "doInit: index file not found or open failed:" << indexFile.fileName();
        emit finished(result);
        return;
    }

    QJsonParseError error;
    QJsonDocument indexDoc = QJsonDocument::fromJson(indexFile.readAll(), &error);
    indexFile.close();

    if (error.error != QJsonParseError::NoError) {
        qCWarning(logConv) << "doInit: parse index json failed:" << error.errorString();
        emit finished(result);
        return;
    }

    QJsonObject rootObj = indexDoc.object();
    if (!rootObj.contains("indexes") || !rootObj["indexes"].isArray()) {
        qCWarning(logConv) << "doInit: index json missing 'indexes' array";
        emit finished(result);
        return;
    }

    // 遍历索引文件中的每条会话记录，加载其文本内容切片
    const QJsonArray indexes = rootObj["indexes"].toArray();
    for (const auto &itemValue : indexes) {
        if (!itemValue.isObject())
            continue;

        ConversationIndexItem item;
        if (!item.fromJson(itemValue.toObject()))
            continue;

        QString convFilePath = m_storagePath + "/" + item.id + ".json";
        ConversationRecordPtr record = ConversationRecord::loadFromFile(convFilePath);
        if (record.isNull())
            continue;

        // 提取文本内容并切片存储，value为切片列表
        result[item.id] = record->extractTextContent();
    }

    qCDebug(logConv) << "doInit: loaded" << result.size() << "conversation indexes";
    emit finished(result);
}

// 在工作线程中更新指定会话的搜索索引
void ConversationSearchWorker::doUpdateIndex(const ConversationRecordPtr &record)
{
    if (record.isNull()) {
        qCWarning(logConv) << "doUpdateIndex: record is null";
        emit updateFinished(QString(), QStringList());
        return;
    }

    QString id = record->id();
    qCDebug(logConv) << "doUpdateIndex:" << id;
    QStringList contents = record->extractTextContent();
    emit updateFinished(id, contents);
}

// 在工作线程中按关键词搜索索引
void ConversationSearchWorker::doSearch(const SearchIndexMap &indexes, const QString &keyword, int resultLength)
{
    SearchIndexMap results;

    qCDebug(logConv) << "doSearch: keyword =" << keyword << "indexCount =" << indexes.size();
    if (keyword.isEmpty()) {
        emit searchFinished(indexes);
        return;
    }

    QString lowerKeyword = keyword.toLower();

    for (auto it = indexes.constBegin(); it != indexes.constEnd(); ++it) {
        const QString &id = it.key();
        const QStringList &textList = it.value();
       QString fullText = textList.join(QString());

        int pos = fullText.toLower().indexOf(lowerKeyword);
        if (pos < 0)
            continue;

        int start = qMax(0, pos - 100);
        int end = qMin(fullText.length(), pos + keyword.length() + resultLength);
        QString snippet = fullText.mid(start, end - start);

        results[id] = QStringList{snippet};
    }

    qCDebug(logConv) << "doSearch: matched" << results.size() << "/" << indexes.size();
    emit searchFinished(results);
}


}
