#ifndef MIGRATIONMANAGER3_H
#define MIGRATIONMANAGER3_H

#include "migrationmanager.h"

namespace uos_ai {

class MigrationManager3 : public MigrationManager
{
public:
    explicit MigrationManager3();
    bool checkMigrations() override;
    bool runMigrations() override;
protected:
    void markCompleted(const QString &name) override;
    void init();
};

}
#endif // MIGRATIONMANAGER3_H
