#include "migrationmanager3.h"
#include "llmmigration.h"
#include "configmigration.h"
#include "conversationmigration.h"

#include <QDir>
#include <QStandardPaths>
#include <QDateTime>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logMigration)

using namespace uos_ai;

MigrationManager3::MigrationManager3() : MigrationManager()
{

}

void MigrationManager3::init()
{
    auto llm = new LlmMigration;
    registerMigration(llm->name(), llm);

    auto config = new ConfigMigration;
    registerMigration(config->name(), config);

    auto conversation = new ConversationMigration;
    registerMigration(conversation->name(), conversation);
}

bool MigrationManager3::checkMigrations()
{
    QString dbDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/db";
    QString dbPath = dbDir + "/basic";
    QString newdbPath = dbDir + "/app";

    if (QFile::exists(newdbPath) || !QFile::exists(dbPath))
        return false;

    bool need = false;
#if 0
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "migration_check_history");
        db.setDatabaseName(dbPath);

        if (db.open()) {
            QSqlQuery query(db);
            if (query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='migration_history'")) {
                if (!query.next())
                    need = true;
            }

            db.close();
        }
    }
    QSqlDatabase::removeDatabase("migration_check_history");
#else
    need = true;
#endif

    return need;
}

bool MigrationManager3::runMigrations()
{
    init();
    return MigrationManager::runMigrations();
}

void MigrationManager3::markCompleted(const QString &name)
{
#if 0
    QString dbDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/db";
    QString dbPath = dbDir + "/basic";

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "migration_mark" + name);
        db.setDatabaseName(dbPath);

        if (!db.open()) {
            qCWarning(logMigration) << "Failed to open database for migration mark:" << db.lastError().text();
        } else {
            QSqlQuery query(db);
            query.exec("CREATE TABLE IF NOT EXISTS migration_history (name VARCHAR(128) UNIQUE PRIMARY KEY, timestamp INTEGER)");

            query.prepare("INSERT OR REPLACE INTO migration_history (name, timestamp) VALUES (?, ?)");
            query.addBindValue(name);
            query.addBindValue(QDateTime::currentSecsSinceEpoch());

            if (!query.exec()) {
                qCWarning(logMigration) << "Failed to mark migration completed:" << query.lastError().text();
            }

            db.close();
        }
    }

    QSqlDatabase::removeDatabase("migration_mark_" + name);
#endif
}
