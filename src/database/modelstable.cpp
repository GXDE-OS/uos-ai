#include "modelstable.h"
#include "sqlitebase.h"
#include "global_key_define.h"

#include <QJsonDocument>
#include <QLoggingCategory>
#include <QDir>

Q_DECLARE_LOGGING_CATEGORY(logDatabase)

using namespace uos_ai;

ModelsTable::ModelsTable()
    : d(new ModelsObject())
{
}

ModelsTable::ModelsTable(const ModelsTable &other)
    : d(other.d)
{
}

ModelsTable::ModelsTable(const ModelsObject &object)
    : d(new ModelsObject(object))
{
}

ModelsTable::~ModelsTable()
{
}

ModelsTable &ModelsTable::operator=(const ModelsTable &other)
{
    d = other.d;
    return *this;
}

QString ModelsTable::id() const
{
    return d->id;
}

void ModelsTable::setId(const QString &id)
{
    d->id = id;
}

QString ModelsTable::provider() const
{
    return d->provider;
}

void ModelsTable::setProvider(const QString &provider)
{
    d->provider = provider;
}

QJsonObject ModelsTable::model() const
{
    QJsonDocument doc = QJsonDocument::fromJson(d->model.toUtf8());
    QJsonObject obj = doc.object();
    return obj;
}

void ModelsTable::setModel(const QJsonObject &modelObj)
{
    d->model = QJsonDocument(modelObj).toJson(QJsonDocument::Compact);
}

QJsonObject ModelsTable::params() const
{
    QJsonDocument doc = QJsonDocument::fromJson(d->params.toUtf8());
    QJsonObject obj = doc.object();
    return obj;
}

void ModelsTable::setParams(const QJsonObject &params)
{
    d->params = QJsonDocument(params).toJson(QJsonDocument::Compact);
}

QJsonObject ModelsTable::additional() const
{
    QJsonDocument doc = QJsonDocument::fromJson(d->additional.toUtf8());
    QJsonObject obj = doc.object();
    return obj;
}

void ModelsTable::setAdditional(const QJsonObject &additional)
{
    d->additional = QJsonDocument(additional).toJson(QJsonDocument::Compact);
}

bool ModelsTable::save(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "INSERT INTO models (id, provider, model, params, additional) VALUES (:id, :provider, :model, :params, :additional)";
    QVariantHash params;
    params[STR_KEY_ID] = d->id;
    params[STR_KEY_PROVIDER] = d->provider;
    params[STR_KEY_MODEL] = d->model;
    params[STR_KEY_PARAMS] = d->params;
    params[STR_KEY_ADDITIONAL] = d->additional;

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to save model:" << db->lastError();
        return false;
    }

    return true;
}

bool ModelsTable::update(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "UPDATE models SET provider=:provider, model=:model, params=:params, additional=:additional WHERE id=:id";
    QVariantHash params;
    params[STR_KEY_ID] = d->id;
    params[STR_KEY_PROVIDER] = d->provider;
    params[STR_KEY_MODEL] = d->model;
    params[STR_KEY_PARAMS] = d->params;
    params[STR_KEY_ADDITIONAL] = d->additional;

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to update model:" << db->lastError();
        return false;
    }

    return true;
}

bool ModelsTable::remove(SQLiteBase *db)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return false;
    }

    QString sql = "DELETE FROM models WHERE id=:id";
    QVariantHash params;
    params[STR_KEY_ID] = id();

    if (!db->executeBatch(sql, params)) {
        qCWarning(logDatabase) << "Failed to delete model:" << db->lastError();
        return false;
    }

    return true;
}

ModelsTable ModelsTable::create(const QString &id, const QString &provider, const QString &model, const QString &params, const QString &additional)
{
    ModelsObject obj;
    obj.id = id;
    obj.provider = provider;
    obj.model = model;
    obj.params = params;
    obj.additional = additional;
    return ModelsTable(obj);
}

ModelsTable ModelsTable::create(const QString &id, const QString &provider, const QJsonObject &model, const QJsonObject &params, const QJsonObject &additional)
{
    ModelsTable table;
    table.setId(id);
    table.setProvider(provider);
    table.setModel(model);
    table.setParams(params);
    table.setAdditional(additional);
    return table;
}

ModelsTable ModelsTable::get(SQLiteBase *db, const QString &id)
{
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return ModelsTable();
    }

    QString sql = QString("SELECT * FROM models WHERE id = '%1'").arg(id);
    QVariantHash record;

    if (!db->queryRecord(sql, record)) {
        qCDebug(logDatabase) << "Model not found for id:" << id;
        return ModelsTable();
    }

    return ModelsTable::create(
        record[STR_KEY_ID].toString(),
        record[STR_KEY_PROVIDER].toString(),
        record[STR_KEY_MODEL].toString(),
        record[STR_KEY_PARAMS].toString(),
        record[STR_KEY_ADDITIONAL].toString()
    );
}

QList<ModelsTable> ModelsTable::getAll(SQLiteBase *db)
{
    QList<ModelsTable> modelsList;
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return modelsList;
    }

    QString sql = "SELECT * FROM models";
    QList<QVariantHash> records;

    if (!db->queryRecords(sql, records)) {
        qCWarning(logDatabase) << "Failed to get all models:" << db->lastError();
        return modelsList;
    }

    for (const auto &record : records) {
        modelsList.append(ModelsTable::create(
            record[STR_KEY_ID].toString(),
            record[STR_KEY_PROVIDER].toString(),
            record[STR_KEY_MODEL].toString(),
            record[STR_KEY_PARAMS].toString(),
            record[STR_KEY_ADDITIONAL].toString()
        ));
    }

    return modelsList;
}

QString ModelsTable::createTableSql()
{
    return "CREATE TABLE IF NOT EXISTS models ("
           "id TEXT PRIMARY KEY,"
           "provider TEXT NOT NULL,"
           "model TEXT NOT NULL,"
           "params TEXT,"
           "additional TEXT"
           ");";
}

QList<ModelsTable> ModelsTable::getByProvider(SQLiteBase *db, const QString &provider)
{
    QList<ModelsTable> modelsList;
    if (!db) {
        qCWarning(logDatabase) << "Database pointer is null";
        return modelsList;
    }

    QString sql = QString("SELECT * FROM models WHERE provider = '%1'").arg(provider);
    QList<QVariantHash> records;

    if (!db->queryRecords(sql, records)) {
        qCWarning(logDatabase) << "Failed to get models by provider:" << db->lastError();
        return modelsList;
    }

    for (const auto &record : records) {
        modelsList.append(ModelsTable::create(
            record[STR_KEY_ID].toString(),
            record[STR_KEY_PROVIDER].toString(),
            record[STR_KEY_MODEL].toString(),
            record[STR_KEY_PARAMS].toString(),
            record[STR_KEY_ADDITIONAL].toString()
        ));
    }

    return modelsList;
}

QJsonObject ModelsTable::createModel(int arch, int ability, const QString &modelId, const QString &name)
{
    QJsonObject modelObj;
    modelObj[STR_KEY_ARCH] = arch;
    modelObj[STR_KEY_ABILITY] = ability;
    modelObj[STR_KEY_MODEL_ID] = modelId;
    modelObj[STR_KEY_NAME] = name;
    return modelObj;
}

QJsonObject ModelsTable::createModel(const QString &id)
{
    QJsonObject modelObj;
    modelObj[STR_KEY_ID] = id;
    return modelObj;
}

QString ModelsTable::tableName()
{
      return QStringLiteral("models");
}
