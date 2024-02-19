#ifndef DBBASE_H
#define DBBASE_H

#include <QPointer>
#include <QMap>

class DaoClient;

class DbBase
{
public:
    DbBase();

    virtual ~DbBase();

    virtual bool save();

    virtual bool remove();

    virtual bool update();

protected:
    QPointer<DaoClient> m_daoClient;
};

#endif // DBBASE_H
