#ifndef MIGRATIONMANAGER_H
#define MIGRATIONMANAGER_H

#include <QString>
#include <QMap>

namespace uos_ai {

class Migration;

class MigrationManager
{
public:
    explicit MigrationManager();
    virtual ~MigrationManager();
    virtual bool checkMigrations() = 0;
    virtual bool runMigrations();

protected:
    virtual void registerMigration(const QString &name, Migration *migration);
    virtual void markCompleted(const QString &name) = 0;
private:
    QMap<QString, Migration *> m_migrations;
};

}

#endif // MIGRATIONMANAGER_H
