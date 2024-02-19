#pragma once
#include <tables/dbbase.h>

#include <QStringList>
#include <QDateTime>
#include <QVariant>
#include <QSharedDataPointer>

class AppObject;

class AppTable : public DbBase
{
public:
    AppTable();
    AppTable(const AppTable  &other);
    AppTable(const AppObject &object);

    ~AppTable();

    AppTable  &operator=(const AppTable  &other);

    const AppObject *modelData() const;

    QString uuid() const;
    void setUuid(const QString &user);

    QString name() const;
    void setName(const QString &pass);

    QString desc() const;
    void setDesc(const QString &desc);

    QString llmid() const;
    void setLlmid(const QString &llmid);

    QString cmd() const;
    void setCmd(const QString &cmd);

    QString ext() const;
    void setExt(const QString &ext);

    virtual bool save() override ;
    virtual bool update() override ;
    virtual bool remove() override ;

    static AppTable  create();

    static AppTable  create(const QString &uuid, const QString &name
                            , const QString &desc, const QString &llmid, const QString &cmd, const QString &ext);

    static AppTable  get(const QString &uuid);

    static QList<AppTable> getAll();

    static int count();

private:
    QSharedDataPointer<AppObject> d;
};

Q_DECLARE_METATYPE(AppTable)
Q_DECLARE_METATYPE(QList<AppTable>)

