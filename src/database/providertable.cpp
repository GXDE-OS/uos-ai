#include "providertable.h"
#include "sqlitebase.h"
#include "global_key_define.h"

#include <QLoggingCategory>
#include <QDir>
#include <QStringLiteral>

Q_DECLARE_LOGGING_CATEGORY(logDatabase)

using namespace uos_ai;

ProviderTable::ProviderTable()
    : d(new ProviderObject())
{
}

ProviderTable::ProviderTable(const ProviderTable &other)
    : d(other.d)
{
}

ProviderTable::ProviderTable(const ProviderObject &object)
    : d(new ProviderObject(object))
{
}

ProviderTable::~ProviderTable()
{
}

ProviderTable &ProviderTable::operator=(const ProviderTable &other)
{
    d = other.d;
    return *this;
}

QString ProviderTable::id() const
{
    return d->id;
}

void ProviderTable::setId(const QString &id)
{
    d->id = id;
}

QString ProviderTable::name() const
{
    return d->name;
}

void ProviderTable::setName(const QString &name)
{
    d->name = name;
}

QString ProviderTable::auth() const
{
    return d->auth;
}

void ProviderTable::setAuth(const QString &auth)
{
    d->auth = auth;
}

QString ProviderTable::provider() const
{
    return d->provider;
}

void ProviderTable::setProvider(const QString &provider)
{
    d->provider = provider;
}

QString ProviderTable::additional() const
{
    return d->additional;
}

void ProviderTable::setAdditional(const QString &additional)
{
    d->additional = additional;
}

bool ProviderTable::save(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "INSERT INTO provider (id, name, auth, provider, additional) VALUES (:id, :name, :auth, :provider, :additional)";
    QVariantHash params;
    params[STR_KEY_ID] = id();
    params[STR_KEY_NAME] = name();
    params[STR_KEY_AUTH] = auth();
    params[STR_KEY_PROVIDER] = provider();
    params[STR_KEY_ADDITIONAL] = additional();

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to save provider:" << db->lastError();
        return false;
    }

    return true;
}

bool ProviderTable::update(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "UPDATE provider SET name=:name, auth=:auth, provider=:provider, additional=:additional WHERE id=:id";
    QVariantHash params;
    params[STR_KEY_NAME] = name();
    params[STR_KEY_AUTH] = auth();
    params[STR_KEY_PROVIDER] = provider();
    params[STR_KEY_ADDITIONAL] = additional();
    params[STR_KEY_ID] = id();

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to update provider:" << db->lastError();
        return false;
    }

    return true;
}

bool ProviderTable::remove(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "DELETE FROM provider WHERE id=:id";
    QVariantHash params;
    params[STR_KEY_ID] = id();

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to delete provider:" << db->lastError();
        return false;
    }

    return true;
}

ProviderTable ProviderTable::create(const QString &id, const QString &name, const QString &auth, const QString &provider, const QString &additional)
{
    ProviderObject obj;
    obj.id = id;
    obj.name = name;
    obj.auth = auth;
    obj.provider = provider;
    obj.additional = additional;
    return ProviderTable(obj);
}

ProviderTable ProviderTable::get(SQLiteBase *db, const QString &id)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return ProviderTable();
    }

    QString sql = QString("SELECT * FROM provider WHERE id = '%1'").arg(id);
    QVariantHash record;

    if (!db->queryRecord(sql, record)) {
        qCDebug(logDatabase) << "Provider not found for id:" << id;
        return ProviderTable();
    }

    return ProviderTable::create(
        record[STR_KEY_ID].toString(),
        record[STR_KEY_NAME].toString(),
        record[STR_KEY_AUTH].toString(),
        record[STR_KEY_PROVIDER].toString(),
        record[STR_KEY_ADDITIONAL].toString()
    );
}

ProviderTable ProviderTable::getByName(SQLiteBase *db, const QString &name)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return ProviderTable();
    }

    QString sql = QString("SELECT * FROM provider WHERE name = '%1'").arg(name);
    QVariantHash record;

    if (!db->queryRecord(sql, record)) {
        qCDebug(logDatabase) << "Provider not found for name:" << name;
        return ProviderTable();
    }

    return ProviderTable::create(
        record[STR_KEY_ID].toString(),
        record[STR_KEY_NAME].toString(),
        record[STR_KEY_AUTH].toString(),
        record[STR_KEY_PROVIDER].toString(),
        record[STR_KEY_ADDITIONAL].toString()
    );
}

QList<ProviderTable> ProviderTable::getAll(SQLiteBase *db)
{
    QList<ProviderTable> providerList;
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return providerList;
    }

    QString sql = "SELECT * FROM provider";
    QList<QVariantHash> records;

    if (!db->queryRecords(sql, records)) {
        qCWarning(logDatabase) << "Failed to get all providers:" << db->lastError();
        return providerList;
    }

    for (const auto &record : records) {
        providerList.append(ProviderTable::create(
            record[STR_KEY_ID].toString(),
            record[STR_KEY_NAME].toString(),
            record[STR_KEY_AUTH].toString(),
            record[STR_KEY_PROVIDER].toString(),
            record[STR_KEY_ADDITIONAL].toString()
        ));
    }

    return providerList;
}

QString ProviderTable::tableName()
{
    return QStringLiteral("provider");
}

QString ProviderTable::createTableSql()
{
    return "CREATE TABLE IF NOT EXISTS provider ("
           "id TEXT PRIMARY KEY,"
           "name TEXT NOT NULL,"
           "auth TEXT,"
           "provider TEXT NOT NULL,"
           "additional TEXT"
           ");";
}