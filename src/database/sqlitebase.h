#ifndef SQLITEBASE_H
#define SQLITEBASE_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QList>
#include <QMap>
#include <QMutex>

namespace uos_ai {

class SQLiteBase
{
public:
    SQLiteBase();
    virtual ~SQLiteBase();

    bool initialize(const QString &dbPath, const QString &dbName);

    void close();

    bool createTable(const QString &tableSql);

    bool tableExists(const QString &tableName);

    bool executeSql(const QString &sql, const QVariantHash &params);

    bool executeBatch(const QString &sql, const QVariantHash &params);

    bool queryRecord(const QString &sql, QVariantHash &record);

    bool queryRecords(const QString &sql, QList<QVariantHash> &records);

    qint64 lastInsertId();

    int affectedRows();

    QString lastError() const;

protected:
    QSqlDatabase m_database;
    QString m_lastError;
    mutable QMutex m_mutex;
};

}

#endif // SQLITEBASE_H
