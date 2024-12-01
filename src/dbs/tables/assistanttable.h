#pragma once
#include <tables/dbbase.h>

#include <QStringList>
#include <QDateTime>
#include <QVariant>
#include <QSharedDataPointer>

class AssistantObject;

class AssistantTable : public DbBase
{
public:
    AssistantTable();
    AssistantTable(const AssistantTable &other);
    AssistantTable(const AssistantObject &object);
    ~AssistantTable();

    QString id() const;
    void setId(const QString &id);

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    int type() const;
    void setType(int nType);

    QString description() const;
    void setDescription(const QString &desc);


    AssistantTable &operator=(const AssistantTable &other);

    virtual bool save()   override ;
    virtual bool update() override ;
    virtual bool remove() override ;

    static AssistantTable create(const QString &id, const QString &displayName, int type, const QString &description);
    static AssistantTable create();

    static AssistantTable get(const QString &id);

    static int count();
    static QList<AssistantTable> getAll();

private:
    QSharedDataPointer<AssistantObject> d;
};

Q_DECLARE_METATYPE(AssistantTable)
Q_DECLARE_METATYPE(QList<AssistantTable>)

