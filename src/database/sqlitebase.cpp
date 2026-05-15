#include "sqlitebase.h"

#include <QSqlRecord>
#include <QSqlField>
#include <QLoggingCategory>
#include <QDir>
#include <QMutexLocker>

Q_DECLARE_LOGGING_CATEGORY(logDatabase)

using namespace uos_ai;

SQLiteBase::SQLiteBase()
{
}

SQLiteBase::~SQLiteBase()
{
    close();
}

bool SQLiteBase::initialize(const QString &dbPath, const QString &dbName)
{
    QMutexLocker locker(&m_mutex);

    if (dbPath.isEmpty() || dbName.isEmpty()) {
        m_lastError = "Database path or name is empty";
        qCWarning(logDatabase) << m_lastError;
        return false;
    }

    QString fullDbPath = dbPath;
    if (!dbPath.endsWith('/')) {
        fullDbPath += '/';
    }
    fullDbPath += dbName;

    QDir dir(dbPath);
    if (!dir.exists()) {
        if (!dir.mkpath(dbPath)) {
            m_lastError = "Failed to create database directory: " + dbPath;
            qCWarning(logDatabase) << m_lastError;
            return false;
        }
    }

    QString connectionName = dbName + "_connection";

    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    m_database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_database.setDatabaseName(fullDbPath);

    if (!m_database.open()) {
        m_lastError = "Failed to open database: " + m_database.lastError().text();
        qCWarning(logDatabase) << m_lastError;
        return false;
    }

    qCDebug(logDatabase) << "Database initialized successfully:" << fullDbPath;
    return true;
}

void SQLiteBase::close()
{
    QMutexLocker locker(&m_mutex);

    if (m_database.isOpen()) {
        m_database.close();
        qCDebug(logDatabase) << "Database closed";
    }
}

bool SQLiteBase::createTable(const QString &tableSql)
{
    QMutexLocker locker(&m_mutex);

    if (!m_database.isOpen()) {
        m_lastError = "Database is not open";
        qCWarning(logDatabase) << m_lastError;
        return false;
    }

    QSqlQuery query(m_database);
    if (!query.exec(tableSql)) {
        m_lastError = "Failed to create table: " + query.lastError().text();
        qCWarning(logDatabase) << m_lastError;
        return false;
    }

    qCDebug(logDatabase) << "Table created successfully";
    return true;
}

bool SQLiteBase::tableExists(const QString &tableName)
{
    QMutexLocker locker(&m_mutex);

    if (!m_database.isOpen()) {
        m_lastError = "Database is not open";
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=?");
    query.addBindValue(tableName);

    if (query.exec() && query.next()) {
        return true;
    }

    return false;
}

bool SQLiteBase::executeSql(const QString &sql, const QVariantHash &params)
{
    QMutexLocker locker(&m_mutex);

    if (!m_database.isOpen()) {
        m_lastError = "Database is not open";
        qCWarning(logDatabase) << m_lastError;
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(sql);

    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        query.bindValue(it.key(), it.value());
    }

    if (!query.exec()) {
        m_lastError = "Failed to execute SQL: " + query.lastError().text();
        qCWarning(logDatabase) << m_lastError << "SQL:" << sql;
        return false;
    }

    qCDebug(logDatabase) << "SQL executed successfully" << (params.isEmpty() ? "" : "with parameters");
    return true;
}

bool SQLiteBase::executeBatch(const QString &sql, const QVariantHash &params)
{
    QMutexLocker locker(&m_mutex);

    if (!m_database.isOpen()) {
        m_lastError = "Database is not open";
        qCWarning(logDatabase) << m_lastError;
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(sql);

    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        query.bindValue(":" + it.key(), it.value());
    }

    if (!query.exec()) {
        m_lastError = "Failed to execute batch SQL: " + query.lastError().text();
        qCWarning(logDatabase) << m_lastError << "SQL:" << sql;
        return false;
    }

    return true;
}

bool SQLiteBase::queryRecord(const QString &sql, QVariantHash &record)
{
    QMutexLocker locker(&m_mutex);

    if (!m_database.isOpen()) {
        m_lastError = "Database is not open";
        return false;
    }

    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        m_lastError = "Failed to query record: " + query.lastError().text();
        qCWarning(logDatabase) << m_lastError;
        return false;
    }

    if (query.next()) {
        QSqlRecord rec = query.record();
        for (int i = 0; i < rec.count(); ++i) {
            record[rec.fieldName(i)] = rec.value(i);
        }
        return true;
    }

    return false;
}

bool SQLiteBase::queryRecords(const QString &sql, QList<QVariantHash> &records)
{
    QMutexLocker locker(&m_mutex);

    if (!m_database.isOpen()) {
        m_lastError = "Database is not open";
        return false;
    }

    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        m_lastError = "Failed to query records: " + query.lastError().text();
        qCWarning(logDatabase) << m_lastError;
        return false;
    }

    records.clear();
    while (query.next()) {
        QVariantHash record;
        QSqlRecord rec = query.record();
        for (int i = 0; i < rec.count(); ++i) {
            record[rec.fieldName(i)] = rec.value(i);
        }
        records.append(record);
    }

    qCDebug(logDatabase) << "Queried" << records.size() << "records";
    return true;
}

qint64 SQLiteBase::lastInsertId()
{
    QMutexLocker locker(&m_mutex);

    if (!m_database.isOpen()) {
        m_lastError = "Database is not open";
        return -1;
    }

    QSqlQuery query(m_database);
    if (query.exec("SELECT last_insert_rowid();")) {
        if (query.next()) {
            return query.value(0).toLongLong();
        }
    }

    return -1;
}

int SQLiteBase::affectedRows()
{
    QMutexLocker locker(&m_mutex);

    if (!m_database.isOpen()) {
        m_lastError = "Database is not open";
        return -1;
    }

    QSqlQuery query(m_database);
    if (query.exec("SELECT changes();")) {
        if (query.next()) {
            return query.value(0).toInt();
        }
    }

    return -1;
}

QString SQLiteBase::lastError() const
{
    return m_lastError;
}
