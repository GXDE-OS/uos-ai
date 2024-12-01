#pragma once
#include <tables/dbbase.h>

#include <QStringList>
#include <QDateTime>
#include <QVariant>
#include <QSharedDataPointer>

class CurLlmObject;

class CurLlmTable : public DbBase
{
public:
    CurLlmTable();
    CurLlmTable(const CurLlmTable &other);
    CurLlmTable(const CurLlmObject &object);
    ~CurLlmTable();

    QString assistantTd() const;
    void setAssistantId(const QString &);

    QString llmId() const;
    void setLlmId(const QString &);

    CurLlmTable &operator=(const CurLlmTable &other);

    virtual bool save()   override ;
    virtual bool update() override ;
    virtual bool remove() override ;

    static CurLlmTable create(const QString &assistantid, const QString &llmid);
    static CurLlmTable create();

    static CurLlmTable get(const QString &assistantId);

    static int count();
    static QList<CurLlmTable> getAll();

private:
    QSharedDataPointer<CurLlmObject> d;
};

Q_DECLARE_METATYPE(CurLlmTable)
Q_DECLARE_METATYPE(QList<CurLlmTable>)

