#ifndef USEDMODELTABLE_H
#define USEDMODELTABLE_H

#include "dbtable.h"

#include <QSharedDataPointer>

class SQLiteBase;

namespace uos_ai {

class UsedModelTable : public DbTable
{
public:
    class UsedModelObject : public QSharedData
    {
    public:
        QString assistant;
        QString model;
        QString additional;
    };
public:
    UsedModelTable();
    UsedModelTable(const UsedModelTable &other);
    UsedModelTable(const UsedModelObject &object);
    ~UsedModelTable();

    UsedModelTable &operator=(const UsedModelTable &other);

    QString assistant() const;
    void setAssistant(const QString &assistant);
    
    QString model() const;
    void setModel(const QString &model);

    QString additional() const;
    void setAdditional(const QString &additional);

    bool save(SQLiteBase *db) override;
    bool update(SQLiteBase *db) override;
    bool remove(SQLiteBase *db) override;

    static UsedModelTable create(const QString &assistant, const QString &model, const QString &additional);
    static UsedModelTable getByAssistant(SQLiteBase *db, const QString &assistant);
    static QList<UsedModelTable> getAll(SQLiteBase *db);

    static QString tableName();
    static QString createTableSql();

private:
    QSharedDataPointer<UsedModelObject> d;
};

}

#endif // USEDMODELTABLE_H
