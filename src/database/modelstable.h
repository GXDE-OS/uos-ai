#ifndef MODELSTABLE_H
#define MODELSTABLE_H

#include "dbtable.h"

#include <QSharedPointer>
#include <QJsonObject>

class SQLiteBase;

namespace uos_ai {

class ModelsTable : public DbTable
{
public:
    class ModelsObject : public QSharedData
    {
    public:
        QString id;
        QString provider;
        QString model;
        QString params;
        QString additional;
    };

public:
    ModelsTable();
    ModelsTable(const ModelsTable &other);
    ModelsTable(const ModelsObject &object);
    ~ModelsTable();

    ModelsTable &operator=(const ModelsTable &other);

    QString id() const;
    void setId(const QString &id);

    QString provider() const;
    void setProvider(const QString &provider);

    QJsonObject model() const;
    void setModel(const QJsonObject &);

    QJsonObject params() const;
    void setParams(const QJsonObject &params);

    QJsonObject additional() const;
    void setAdditional(const QJsonObject &additional);

    bool save(SQLiteBase *db) override;
    bool update(SQLiteBase *db) override;
    bool remove(SQLiteBase *db) override;

    static ModelsTable create(const QString &id, const QString &provider, const QString &model, const QString &params = "", const QString &additional = "");
    static ModelsTable create(const QString &id, const QString &provider, const QJsonObject &model, const QJsonObject &params = {}, const QJsonObject &additional = {});
    static ModelsTable get(SQLiteBase *db, const QString &id);
    static QList<ModelsTable> getAll(SQLiteBase *db);
    static QList<ModelsTable> getByProvider(SQLiteBase *db, const QString &provider);

    static QJsonObject createModel(int arch, int ability, const QString &modelId, const QString &name);
    static QJsonObject createModel(const QString &id);
    static QString tableName();

    static QString createTableSql();

private:
    QSharedDataPointer<ModelsObject> d;
};

}

#endif // MODELSTABLE_H
