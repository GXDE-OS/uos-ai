#ifndef MIGRATION_H
#define MIGRATION_H

#include <QString>

namespace uos_ai {

class Migration
{
public:
    virtual ~Migration() = default;

    virtual QString name() const = 0;
    virtual bool migrate() = 0;
    virtual bool isNeeded() const = 0;
};

}

#endif // MIGRATION_H
