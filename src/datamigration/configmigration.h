#ifndef CONFIGMIGRATION_H
#define CONFIGMIGRATION_H

#include "migration.h"

namespace uos_ai {

class ConfigMigration : public Migration
{
public:
    QString name() const override;
    bool migrate() override;
    bool isNeeded() const override;

private:
    bool migrateConfigData();
};

}

#endif // CONFIGMIGRATION_H
