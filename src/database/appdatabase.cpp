#include "appdatabase.h"
#include "appconfigtable.h"
#include "usedmodeltable.h"
#include "providertable.h"
#include "modelstable.h"
#include "global_key_define.h"
#include <QStandardPaths>
#include <QDir>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>

Q_DECLARE_LOGGING_CATEGORY(logDatabase)

using namespace uos_ai;

AppDatabase::AppDatabase()
    : m_dbName("app")
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_dbPath = appDataPath + "/db";

    QDir dir(m_dbPath);
    if (!dir.exists()) {
        dir.mkpath(m_dbPath);
    }
}

AppDatabase::~AppDatabase()
{
    close();
}

AppDatabase *AppDatabase::instance()
{
    static AppDatabase instance;
    return &instance;
}

bool AppDatabase::init()
{
    QString fullDbPath = m_dbPath + "/" + m_dbName;

    qCDebug(logDatabase) << "Initializing AppDatabase at:" << fullDbPath;

    if (!m_database.isOpen()) {
        if (!initialize(m_dbPath, m_dbName)) {
            qCCritical(logDatabase) << "Failed to initialize AppDatabase:" << lastError();
            return false;
        }
    }

    if (!createTables()) {
        qCCritical(logDatabase) << "Failed to create tables";
        return false;
    }

    qCInfo(logDatabase) << "AppDatabase initialized successfully";
    return true;
}

QString AppDatabase::getDatabasePath() const
{
    return m_dbPath + "/" + m_dbName;
}

bool AppDatabase::createTables()
{
    // 检查并创建 config 表
    if (!tableExists(AppConfigTable::tableName())) {
        qCDebug(logDatabase) << "Creating config table...";
        if (!createTable(AppConfigTable::createTableSql())) {
            qCCritical(logDatabase) << "Failed to create config table";
            return false;
        }
        qCDebug(logDatabase) << "Config table created successfully";
    } else {
        qCDebug(logDatabase) << "Config table already exists";
    }

    // 检查并创建 usedModel 表
    if (!tableExists(UsedModelTable::tableName())) {
        qCDebug(logDatabase) << "Creating usedModel table...";
        if (!createTable(UsedModelTable::createTableSql())) {
            qCCritical(logDatabase) << "Failed to create usedModel table";
            return false;
        }
        qCDebug(logDatabase) << "UsedModel table created successfully";
    } else {
        qCDebug(logDatabase) << "UsedModel table already exists";
    }

    // 检查并创建 provider 表
    if (!tableExists(ProviderTable::tableName())) {
        qCDebug(logDatabase) << "Creating provider table...";
        if (!createTable(ProviderTable::createTableSql())) {
            qCCritical(logDatabase) << "Failed to create provider table";
            return false;
        }
        qCDebug(logDatabase) << "Provider table created successfully";
    } else {
        qCDebug(logDatabase) << "Provider table already exists";
    }

    // 检查并创建 models 表
    if (!tableExists(ModelsTable::tableName())) {
        qCDebug(logDatabase) << "Creating models table...";
        if (!createTable(ModelsTable::createTableSql())) {
            qCCritical(logDatabase) << "Failed to create models table";
            return false;
        }
        qCDebug(logDatabase) << "Models table created successfully";
    } else {
        qCDebug(logDatabase) << "Models table already exists";
    }

    qCDebug(logDatabase) << "All tables checked and created successfully";
    return true;
}

QMap<QString, ProviderAccount> AppDatabase::queryAllProviders()
{
    QMap<QString, ProviderAccount> providers;
    QList<ProviderTable> providerTables = ProviderTable::getAll(this);

    for (const auto &providerTable : providerTables) {
        ProviderAccount account;
        account.id = providerTable.id();
        account.name = providerTable.name();
        account.provider = providerTable.provider();

        QJsonDocument authDoc = QJsonDocument::fromJson(providerTable.auth().toUtf8());
        account.auth = authDoc.object().toVariantHash();

        QJsonDocument additionalDoc = QJsonDocument::fromJson(providerTable.additional().toUtf8());
        account.additional = additionalDoc.object().toVariantHash();

        providers.insert(account.id, account);
    }

    return providers;
}

QList<ModelAccountPtr> AppDatabase::queryAllModels()
{
    QList<ModelAccountPtr> models;
    QList<ModelsTable> modelTables = ModelsTable::getAll(this);

    for (const auto &modelTable : modelTables) {
        ModelAccountPtr modelAcc(new ModelAccount);
        modelAcc->id = modelTable.id();
        modelAcc->account.id = modelTable.provider();

        QJsonObject modelObj = modelTable.model();

        if (modelObj.contains(STR_KEY_ID) && !modelObj[STR_KEY_ID].toString().isEmpty()) {
            modelAcc->model.id = modelObj[STR_KEY_ID].toString();
        } else {
            modelAcc->model.arch = static_cast<ModelArch>(modelObj[STR_KEY_ARCH].toInt());
            modelAcc->model.ability = static_cast<ModelAbilities>(modelObj[STR_KEY_ABILITY].toInt());
            modelAcc->model.modelId = modelObj[STR_KEY_MODEL_ID].toString();
            modelAcc->model.name = modelObj[STR_KEY_NAME].toString();
        }

        modelAcc->params = modelTable.params().toVariantHash();

        models.append(modelAcc);
    }

    return models;
}

void AppDatabase::deleteProvider(const QString &id)
{
    QList<ModelsTable> models = ModelsTable::getByProvider(this, id);
    for (auto &model : models) {
        if (!model.remove(this)) {
            qCCritical(logDatabase) << "Failed to delete model:" << model.id();
            continue;
        }
    }

    ProviderTable provider = ProviderTable::get(this, id);
    if (!provider.remove(this)) {
        qCCritical(logDatabase) << "Failed to delete provider:" << id;
    }

    return;
}

void AppDatabase::deleteModel(const QString &id)
{
    ModelsTable model = ModelsTable::get(this, id);
    if (!model.remove(this)) {
        qCCritical(logDatabase) << "Failed to delete model:" << id;
    }

    return;
}

QString AppDatabase::getConfig(const QString &key)
{
    AppConfigTable config = AppConfigTable::getByName(this, key);
    return config.value();
}

void AppDatabase::saveConfig(const QString &key, const QString &value)
{
    AppConfigTable config = AppConfigTable::getByName(this, key);

    if (!config.name().isEmpty()) {
        // Update existing record
        config.setValue(value);
        if (!config.update(this)) {
            qCWarning(logDatabase) << "Failed to update config:" << lastError();    
        }
    } else {
        // Insert new record
        AppConfigTable newConfig;
        newConfig.setName(key);
        newConfig.setValue(value);
        newConfig.setAdditional("");
        if (!newConfig.save(this)) {
            qCWarning(logDatabase) << "Failed to save window mode:" << lastError();
        }
    }
}
