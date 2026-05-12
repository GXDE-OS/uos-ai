#ifndef LLMMIGRATION_H
#define LLMMIGRATION_H

#include "migration.h"

namespace uos_ai {

class LlmMigration : public Migration
{
public:
    QString name() const override;
    bool migrate() override;
    bool isNeeded() const override;

private:
    bool migrateLlmData();
};

}

#endif // LLMMIGRATION_H
