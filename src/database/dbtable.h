#ifndef DBTABLE_H
#define DBTABLE_H

#include <QString>
#include "sqlitebase.h"

namespace uos_ai {

class DbTable
{
public:
    DbTable() = default;
    virtual ~DbTable() = default;

    virtual bool save(SQLiteBase *db) = 0;
    virtual bool update(SQLiteBase *db) = 0;
    virtual bool remove(SQLiteBase *db) = 0;
};

}

#endif // DBTABLE_H
