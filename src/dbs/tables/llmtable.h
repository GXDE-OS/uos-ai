#pragma once
#include <tables/dbbase.h>

#include <QStringList>
#include <QDateTime>
#include <QVariant>
#include <QSharedDataPointer>

class LlmObject;

class LlmTable : public DbBase
{
public:
    LlmTable();
    LlmTable(const LlmTable &other);
    LlmTable(const LlmObject &object);
    ~LlmTable();

    QString uuid() const;
    void setUuid(const QString &uuid);

    QString name() const;
    void setName(const QString &name);

    int type() const;
    void setType(int nType);

    QString desc() const;
    void setDesc(const QString &desc);

    QString accountProxy() const;
    void setAccountProxy(const QString &accountProxy);

    QString ext() const;
    void setExt(const QString &ext);

    LlmTable &operator=(const LlmTable &other);

    virtual bool save()   override ;
    virtual bool update() override ;
    virtual bool remove() override ;

    static LlmTable create(const QString &uuid, const QString &name
                           , int type, const QString &desc, const QString &accountProxy, const QString &ext);
    static LlmTable create();

    static LlmTable get(const QString &uuid);

    static int count();
    static QList<LlmTable> getAll();

private:
    QSharedDataPointer<LlmObject> d;
};

Q_DECLARE_METATYPE(LlmTable)
Q_DECLARE_METATYPE(QList<LlmTable>)

