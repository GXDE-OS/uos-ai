#include "configmigration.h"
#include "database/appdatabase.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logMigration)

using namespace uos_ai;

QString ConfigMigration::name() const
{
    return "config_migration";
}

bool ConfigMigration::isNeeded() const
{
    return true;
}

bool ConfigMigration::migrate()
{
    return migrateConfigData();
}

bool ConfigMigration::migrateConfigData()
{
    QString dbDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/db";
    QString oldDbPath = dbDir + "/basic";
    
    AppDatabase *newDb = AppDatabase::instance();
    
    {
        QSqlDatabase oldDb = QSqlDatabase::addDatabase("QSQLITE", "config_migration_old");
        oldDb.setDatabaseName(oldDbPath);

        if (!oldDb.open()) {
            qCCritical(logMigration) << "Failed to open old database:" << oldDb.lastError().text();
            return false;
        }

        QSqlQuery query(oldDb);
        if (!query.exec("SELECT type, value FROM config WHERE type IN (1, 10)")) {
            qCCritical(logMigration) << "Failed to query old config data:" << query.lastError().text();
            oldDb.close();
            QSqlDatabase::removeDatabase("config_migration_old");
            return false;
        }

        while (query.next()) {
            int type = query.value("type").toInt();
            QString value = query.value("value").toString();
            
            if (type == 1) { // CopilotSwitch = 1
                newDb->saveConfig(CONFIG_APP_AGREEMENT, value);
                qCInfo(logMigration) << "Migrated config: CopilotSwitch -> CONFIG_APP_AGREEMENT, value:" << value;
            } else if (type == 10) { // CopilotThirdPartyMcp = 10
                newDb->saveConfig(CONFIG_THIRD_PARTY_MCP, value);
                qCInfo(logMigration) << "Migrated config: CopilotThirdPartyMcp -> CONFIG_THIRD_PARTY_MCP, value:" << value;
            }
        }

        oldDb.close();
    }

    QSqlDatabase::removeDatabase("config_migration_old");

    qCInfo(logMigration) << "Config migration completed";
    return true;
}
