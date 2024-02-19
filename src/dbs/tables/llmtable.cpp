#include "llmtable.h"
#include "objects/llmobject.h"
#include "daoclient.h"

#include <QDebug>

LlmTable::LlmTable()
    : d(new LlmObject())
{

}

LlmTable::LlmTable(const LlmTable &other)
    : d(other.d)
{
}

LlmTable::LlmTable(const LlmObject &object)
    : d(new LlmObject(object))
{
}

LlmTable::~LlmTable()
{

}

LlmTable &LlmTable::operator=(const LlmTable &other)
{
    d = other.d;
    return *this;
}


QString LlmTable::uuid() const
{
    return d->uuid;
}

void LlmTable::setUuid(const QString &uuid)
{
    d->uuid = uuid;
}

QString LlmTable::name() const
{
    return d->name;
}

void LlmTable::setName(const QString &name)
{
    d->name = name;
}

int LlmTable::type() const
{
    return d->type;
}

void LlmTable::setType(int nType)
{
    d->type = nType;
}

QString LlmTable::desc() const
{
    return d->desc;
}

void LlmTable::setDesc(const QString &desc)
{
    d->desc = desc;
}

QString LlmTable::accountProxy() const
{
    return d->account_proxy;
}

void LlmTable::setAccountProxy(const QString &accountProxy)
{
    d->account_proxy = accountProxy;
}

QString LlmTable::ext() const
{
    return d->ext;
}

void LlmTable::setExt(const QString &ext)
{
    d->ext = ext;
}

bool LlmTable::save()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
                "INSERT OR REPLACE INTO llm "
                "(uuid, name, type, desc, account_proxy, ext) VALUES (:uuid, :name, :type, :desc, :account_proxy, :ext) ;"
    , {}, {{"uuid", uuid()}, {"name", name()}, {"type", type()}, {"desc", desc()}, {"account_proxy", accountProxy()}, {"ext", ext()}
    }, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}

bool LlmTable::update()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "UPDATE llm SET name=:name, type=:type, desc=:desc, account_proxy=:account_proxy, ext=:ext WHERE uuid=:uuid ;", {{"uuid", uuid()}}
    , {{"uuid", uuid()}, {"name", name()}, {"type", type()}, {"desc", desc()}, {"account_proxy", accountProxy()}, {"ext", ext()}}, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}

bool LlmTable::remove()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "DELETE FROM llm WHERE uuid=:uuid ;", {{"uuid", uuid()}}, {}, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}


LlmTable LlmTable::create(const QString &uuid, const QString &name, int type, const QString &desc, const QString &accountProxy, const QString &ext)
{
    LlmObject obj;
    obj.uuid = uuid;
    obj.name = name;
    obj.type = type;
    obj.desc = desc;
    obj.account_proxy = accountProxy;
    obj.ext = ext;
    return LlmTable(obj);
}

LlmTable LlmTable::create()
{
    return LlmTable();
}

LlmTable LlmTable::get(const QString &uuid)
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "SELECT * FROM llm WHERE uuid=:uuid;", {{"uuid", uuid}}, {}, result, msg, "basic")) {
        if (result && result->size() > 0) {
            auto item = result->value(0);
            auto obj = LlmTable::create(
                           item.value("uuid").toString(), item.value("name").toString(), item.value("type").toInt(), item.value("desc").toString()
                           , item.value("account_proxy").toString(), item.value("ext").toString());
            return obj;
        }
    } else {
        qInfo() << "sql error: " << msg;
    }
    return LlmTable {};
}

int LlmTable::count()
{
    return 0;
}

QList<LlmTable> LlmTable::getAll()
{
    QList<LlmTable> appList;
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execSync("SELECT * FROM llm; ", result, msg, "basic")) {
        if (result) {
            for (auto &iter : * result) {
                auto obj = LlmTable::create(iter.value("uuid").toString(), iter.value("name").toString(), iter.value("type").toInt()
                                            , iter.value("desc").toString(), iter.value("account_proxy").toString(), iter.value("ext").toString());
                appList.append(obj);
            }
        }
    } else {
        qInfo() << "sql error: " << msg;
    }
    return appList;
}

