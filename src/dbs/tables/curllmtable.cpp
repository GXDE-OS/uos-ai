#include "curllmtable.h"
#include "objects/curllmobject.h"
#include "daoclient.h"

#include <QDebug>

CurLlmTable::CurLlmTable()
    : d(new CurLlmObject())
{

}

CurLlmTable::CurLlmTable(const CurLlmTable &other)
    : d(other.d)
{
}

CurLlmTable::CurLlmTable(const CurLlmObject &object)
    : d(new CurLlmObject(object))
{
}

CurLlmTable::~CurLlmTable()
{

}

CurLlmTable &CurLlmTable::operator=(const CurLlmTable &other)
{
    d = other.d;
    return *this;
}

QString CurLlmTable::assistantTd() const
{
    return d->assistantid;
}

void CurLlmTable::setAssistantId(const QString &assistantid)
{
    d->assistantid = assistantid;
}

QString CurLlmTable::llmId() const
{
    return d->llmid;
}

void CurLlmTable::setLlmId(const QString &llmId)
{
    d->llmid = llmId;
}

bool CurLlmTable::save()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
                "INSERT OR REPLACE INTO curllm "
                "(assistantid, llmid) VALUES (:assistantid, :llmid) ;"
    , {}, { {"assistantid", assistantTd()}, {"llmid", llmId()} }, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}

bool CurLlmTable::update()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "UPDATE curllm SET assistantid=:assistantid, llmid=:llmid WHERE assistantid=:assistantid ;", {{"assistantid", assistantTd()}}
    , {{"llmid", llmId()}}, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}

bool CurLlmTable::remove()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "DELETE FROM curllm WHERE assistantid=:assistantid ;", {{"assistantid", assistantTd()}}, {}, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}

CurLlmTable CurLlmTable::create(const QString &assistantid, const QString &llmid)
{
    CurLlmObject obj;
    obj.assistantid = assistantid;
    obj.llmid = llmid;
    return CurLlmTable(obj);
}

CurLlmTable CurLlmTable::create()
{
    return CurLlmTable();
}

CurLlmTable CurLlmTable::get(const QString &assistantId)
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "SELECT * FROM curllm WHERE assistantid=:assistantid;", {{"assistantid", assistantId}}, {}, result, msg, "basic")) {
        if (result && result->size() > 0) {
            auto item = result->value(0);
            auto obj = CurLlmTable::create(
                           item.value("assistantid").toString(), item.value("llmid").toString());
            return obj;
        }
    } else {
        qInfo() << "sql error: " << msg;
    }
    return CurLlmTable {};
}

int CurLlmTable::count()
{
    return 0;
}

QList<CurLlmTable> CurLlmTable::getAll()
{
    QList<CurLlmTable> llmList;
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execSync("SELECT * FROM curllm; ", result, msg, "basic")) {
        if (result) {
            for (auto &iter : * result) {
                auto obj = CurLlmTable::create(iter.value("assistantid").toString(), iter.value("llmid").toString());
                llmList.append(obj);
            }
        }
    } else {
        qInfo() << "sql error: " << msg;
    }
    return llmList;
}

