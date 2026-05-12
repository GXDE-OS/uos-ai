#ifndef APPCONFIGTABLE_H
#define APPCONFIGTABLE_H

#include "dbtable.h"

#include <QSharedPointer>

class SQLiteBase;

namespace uos_ai {

class AppConfigObject : public QSharedData
{
public:
    QString name;
    QString value;
    QString additional;
};

class AppConfigTable : public DbTable
{
public:
    AppConfigTable();
    AppConfigTable(const AppConfigTable &other);
    AppConfigTable(const AppConfigObject &object);
    ~AppConfigTable();

    AppConfigTable &operator=(const AppConfigTable &other);

    QString name() const;
    void setName(const QString &name);

    QString value() const;
    void setValue(const QString &value);

    QString additional() const;
    void setAdditional(const QString &additional);

    bool save(SQLiteBase *db) override;
    bool update(SQLiteBase *db) override;
    bool remove(SQLiteBase *db) override;

    static AppConfigTable create(const QString &name, const QString &value, const QString &additional);
    static AppConfigTable getByName(SQLiteBase *db, const QString &name);
    static QList<AppConfigTable> getAll(SQLiteBase *db);

    static QString tableName();
    static QString createTableSql();

private:
    QSharedDataPointer<AppConfigObject> d;
};

}

#endif // CONFIGTABLE_H
