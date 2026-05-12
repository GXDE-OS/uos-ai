#include "usedmodeltable.h"
#include "sqlitebase.h"
#include "global_key_define.h"
#include <QLoggingCategory>
#include <QDir>

Q_DECLARE_LOGGING_CATEGORY(logDatabase)

using namespace uos_ai;

UsedModelTable::UsedModelTable()
    : d(new UsedModelObject())
{
}

UsedModelTable::UsedModelTable(const UsedModelTable &other)
    : d(other.d)
{
}

UsedModelTable::UsedModelTable(const UsedModelObject &object)
    : d(new UsedModelObject(object))
{
}

UsedModelTable::~UsedModelTable()
{
}

UsedModelTable &UsedModelTable::operator=(const UsedModelTable &other)
{
    d = other.d;
    return *this;
}

QString UsedModelTable::assistant() const
{
    return d->assistant;
}

void UsedModelTable::setAssistant(const QString &assistant)
{
    d->assistant = assistant;
}

QString UsedModelTable::model() const
{
    return d->model;
}

void UsedModelTable::setModel(const QString &model)
{
    d->model = model;
}

QString UsedModelTable::additional() const
{
    return d->additional;
}

void UsedModelTable::setAdditional(const QString &additional)
{
    d->additional = additional;
}

bool UsedModelTable::save(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "INSERT OR REPLACE INTO usedModel (assistant, model, additional) VALUES (:assistant, :model, :additional)";
    QVariantHash params;
    params[STR_KEY_ASSISTANT] = assistant();
    params[STR_KEY_MODEL] = model();
    params[STR_KEY_ADDITIONAL] = additional();

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to save usedModel:" << db->lastError();
        return false;
    }

    return true;
}

bool UsedModelTable::update(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "UPDATE usedModel SET model=:model, additional=:additional WHERE assistant=:assistant";
    QVariantHash params;
    params[STR_KEY_MODEL] = model();
    params[STR_KEY_ADDITIONAL] = additional();
    params[STR_KEY_ASSISTANT] = assistant();

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to update usedModel:" << db->lastError();
        return false;
    }

    return true;
}

bool UsedModelTable::remove(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "DELETE FROM usedModel WHERE assistant=:assistant";
    QVariantHash params;
    params[STR_KEY_ASSISTANT] = assistant();

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to delete usedModel:" << db->lastError();
        return false;
    }

    return true;
}

UsedModelTable UsedModelTable::create(const QString &assistant, const QString &model, const QString &additional)
{
    UsedModelObject obj;
    obj.assistant = assistant;
    obj.model = model;
    obj.additional = additional;
    return UsedModelTable(obj);
}

UsedModelTable UsedModelTable::getByAssistant(SQLiteBase *db, const QString &assistant)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return UsedModelTable();
    }

    QString sql = QString("SELECT * FROM usedModel WHERE assistant = '%1'").arg(assistant);
    QVariantHash record;

    if (!db->queryRecord(sql, record)) {
        qCDebug(logDatabase) << "UsedModel not found for assistant:" << assistant;
        return UsedModelTable();
    }

    return UsedModelTable::create(
        record[STR_KEY_ASSISTANT].toString(),
        record[STR_KEY_MODEL].toString(),
        record[STR_KEY_ADDITIONAL].toString()
    );
}

QList<UsedModelTable> UsedModelTable::getAll(SQLiteBase *db)
{
    QList<UsedModelTable> usedModelList;
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return usedModelList;
    }

    QString sql = "SELECT * FROM usedModel";
    QList<QVariantHash> records;

    if (!db->queryRecords(sql, records)) {
        qCWarning(logDatabase) << "Failed to get all usedModels:" << db->lastError();
        return usedModelList;
    }

    for (const auto &record : records) {
        usedModelList.append(UsedModelTable::create(
            record[STR_KEY_ASSISTANT].toString(),
            record[STR_KEY_MODEL].toString(),
            record[STR_KEY_ADDITIONAL].toString()
        ));
    }

    return usedModelList;
}

QString UsedModelTable::tableName()
{
     return QStringLiteral("usedModel");
}

QString UsedModelTable::createTableSql()
{
    return "CREATE TABLE IF NOT EXISTS usedModel ("
           "assistant TEXT PRIMARY KEY,"
           "model TEXT NOT NULL,"
           "additional TEXT"
           ");";
}
