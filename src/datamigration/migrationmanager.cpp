#include "migrationmanager.h"
#include "migration.h"

#include <QLoggingCategory>
#include <QElapsedTimer>

Q_DECLARE_LOGGING_CATEGORY(logMigration)

using namespace uos_ai;

MigrationManager::MigrationManager()
{

}

MigrationManager::~MigrationManager()
{
    qDeleteAll(m_migrations.values());
}

void MigrationManager::registerMigration(const QString &name, Migration *migration)
{
    m_migrations[name] = migration;
}

bool MigrationManager::runMigrations()
{
    QElapsedTimer t;
    t.start();

    qCInfo(logMigration) << "Start running migrations";
    for (auto it = m_migrations.constBegin(); it != m_migrations.constEnd(); ++it) {
        Migration *migration = it.value();

        if (!migration->isNeeded())
            continue;

        qCInfo(logMigration) << "Running migration:" << migration->name() << "time:" << t.elapsed();

        if (migration->migrate()) {
            markCompleted(migration->name());
            qCInfo(logMigration) << "Migration completed:" << migration->name() << "time:" << t.elapsed();
        } else {
            qCCritical(logMigration) << "Migration failed:" << migration->name() << "time:" << t.elapsed();
        }
    }

    qCInfo(logMigration) << "All migrations completed" << "time:" << t.elapsed();
    return true;
}
