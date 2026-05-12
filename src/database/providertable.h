#ifndef PROVIDERTABLE_H
#define PROVIDERTABLE_H

#include "dbtable.h"

#include <QSharedPointer>

class SQLiteBase;

namespace uos_ai {

class ProviderTable : public DbTable
{
public:
    class ProviderObject : public QSharedData
    {
    public:
        QString id;
        QString name;
        QString auth;
        QString provider;
        QString additional;
    };
public:
    ProviderTable();
    ProviderTable(const ProviderTable &other);
    ProviderTable(const ProviderObject &object);
    ~ProviderTable();

    ProviderTable &operator=(const ProviderTable &other);

    QString id() const;
    void setId(const QString &id);

    QString name() const;
    void setName(const QString &name);

    QString auth() const;
    void setAuth(const QString &auth);

    QString provider() const;
    void setProvider(const QString &provider);

    QString additional() const;
    void setAdditional(const QString &additional);

    bool save(SQLiteBase *db) override;
    bool update(SQLiteBase *db) override;
    bool remove(SQLiteBase *db) override;

    static ProviderTable create(const QString &id, const QString &name, const QString &auth, const QString &provider, const QString &additional);
    static ProviderTable get(SQLiteBase *db, const QString &id);
    static ProviderTable getByName(SQLiteBase *db, const QString &name);
    static QList<ProviderTable> getAll(SQLiteBase *db);

    static QString tableName();
    static QString createTableSql();

private:
    QSharedDataPointer<ProviderObject> d;
};

}

#endif // PROVIDERTABLE_H
