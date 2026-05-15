#include "appconfigtable.h"
#include "sqlitebase.h"
#include "global_key_define.h"

#include <QLoggingCategory>
#include <QDir>

Q_DECLARE_LOGGING_CATEGORY(logDatabase)

using namespace uos_ai;

AppConfigTable::AppConfigTable()
    : d(new AppConfigObject())
{
}

AppConfigTable::AppConfigTable(const AppConfigTable &other)
    : d(other.d)
{
}

AppConfigTable::AppConfigTable(const AppConfigObject &object)
    : d(new AppConfigObject(object))
{
}

AppConfigTable::~AppConfigTable()
{
}

AppConfigTable &AppConfigTable::operator=(const AppConfigTable &other)
{
    d = other.d;
    return *this;
}

QString AppConfigTable::name() const
{
    return d->name;
;
}

void AppConfigTable::setName(const QString &name)
{
    d->name = name;
}

QString AppConfigTable::value() const
{
    return d->value;
}

void AppConfigTable::setValue(const QString &value)
{
    d->value = value;
}

QString AppConfigTable::additional() const
{
    return d->additional;
}

void AppConfigTable::setAdditional(const QString &additional)
{
    d->additional = additional;
}

bool AppConfigTable::save(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "INSERT INTO config (name, value, additional) VALUES (:name, :value, :additional)";
    QVariantHash params;
    params[STR_KEY_NAME] = name();
    params[STR_KEY_VALUE] = value();
    params[STR_KEY_ADDITIONAL] = additional();

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to save config:" << db->lastError();
        return false;
    }

    return true;
}

bool AppConfigTable::update(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "UPDATE config SET value=:value, additional=:additional WHERE name=:name";
    QVariantHash params;
    params[STR_KEY_NAME] = name();
    params[STR_KEY_VALUE] = value();
    params[STR_KEY_ADDITIONAL] = additional();

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to update config:" << db->lastError();
        return false;
    }

    return true;
}

bool AppConfigTable::remove(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "DELETE FROM config WHERE name=:name";
    QVariantHash params;
    params[STR_KEY_NAME] = name();

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to delete config:" << db->lastError();
        return false;
    }

    return true;
}

AppConfigTable AppConfigTable::create(const QString &name, const QString &value, const QString &additional)
{
    AppConfigObject obj;
    obj.name = name;
    obj.value = value;
    obj.additional = additional;
    return AppConfigTable(obj);
}

AppConfigTable AppConfigTable::getByName(SQLiteBase *db, const QString &name)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return AppConfigTable();
    }

    QString sql = QString("SELECT * FROM config WHERE name = '%1'").arg(name);
    QVariantHash record;

    if (!db->queryRecord(sql, record)) {
        qCDebug(logDatabase) << "Config not found for name:" << name;
        return AppConfigTable();
    }

    return AppConfigTable::create(
        record[STR_KEY_NAME].toString(),
        record[STR_KEY_VALUE].toString(),
        record[STR_KEY_ADDITIONAL].toString()
    );
}

QList<AppConfigTable> AppConfigTable::getAll(SQLiteBase *db)
{
    QList<AppConfigTable> configList;
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return configList;
    }

    QString sql = "SELECT * FROM config";
    QList<QVariantHash> records;

    if (!db->queryRecords(sql, records)) {
        qCWarning(logDatabase) << "Failed to get all configs:" << db->lastError();
        return configList;
    }

    for (const auto &record : records) {
        configList.append(AppConfigTable::create(
            record[STR_KEY_NAME].toString(),
            record[STR_KEY_VALUE].toString(),
            record[STR_KEY_ADDITIONAL].toString()
        ));
    }

    return configList;
}

QString AppConfigTable::tableName()
{
    return QStringLiteral("config");
}

QString AppConfigTable::createTableSql()
{
    return "CREATE TABLE IF NOT EXISTS config ("
           "name TEXT PRIMARY KEY,"
           "value TEXT,"
           "additional TEXT"
           ");";
}
