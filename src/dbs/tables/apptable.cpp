#include "apptable.h"
#include "objects/appobject.h"
#include "daoclient.h"
#include <QDebug>

AppTable::AppTable()
    : DbBase()
    , d(new AppObject())
{

}

AppTable::AppTable(const AppTable &other)
    : d(other.d)
{

}

AppTable::AppTable(const AppObject &object)
    : d(new AppObject(object))
{

}

AppTable::~AppTable()
{
}

QString AppTable::uuid() const
{
    return d->uuid;
}

void AppTable::setUuid(const QString &uuid)
{
    d->uuid = uuid;
}

QString AppTable::name() const
{
    return d->name;
}

void AppTable::setName(const QString &name)
{
    d->name = name;
}

QString AppTable::desc() const
{
    return d->desc;
}

void AppTable::setDesc(const QString &desc)
{
    d->desc = desc;
}

QString AppTable::llmid() const
{
    return d->llmid;
}

void AppTable::setLlmid(const QString &llmid)
{
    d->llmid = llmid;
}

QString AppTable::cmd() const
{
    return d->cmd;
}

void AppTable::setCmd(const QString &cmd)
{
    d->cmd = cmd;
}

QString AppTable::ext() const
{
    return d->ext;
}

void AppTable::setExt(const QString &ext)
{
    d->ext = ext;
}

QString AppTable::assistantid() const
{
    return d->assistantid;
}

void AppTable::setAssistantid(const QString &assistantid)
{
    d->assistantid = assistantid;
}

AppTable &AppTable::operator=(const AppTable &other)
{
    d = other.d;
    return *this;
}

bool AppTable::save()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
                "INSERT OR REPLACE INTO app (uuid, name, desc, llmid, cmd, ext, assistantid) VALUES (:uuid, :name, :desc, :llmid, :cmd, :ext, :assistantid) ;", {}
                , {{"uuid", uuid()}, {"name", name()}, {"desc", desc()}, {"llmid", llmid()}, {"cmd", cmd()}, {"assistantid", assistantid()}}, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}

bool AppTable::update()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "UPDATE app SET name=:name, desc=:desc, llmid=:llmid, cmd=:cmd, ext=:ext, assistantid=:assistantid WHERE uuid=:uuid ;", {{"uuid", uuid()}}
    , {{"name", name()}, {"desc", desc()}, {"llmid", llmid()}, {"cmd", cmd()}, {"cmd", ext()}, {"assistantid", assistantid()}}, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}

bool AppTable::remove()
{
    DaoResultListPtr result = nullptr;
    QString msg;

    if (DaoClient::getInstance().execBatchSync(
    "DELETE FROM app WHERE uuid=:uuid ;", {{"uuid", uuid()}}, {}, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}

AppTable  AppTable::create()
{
    return AppTable();
}

AppTable  AppTable::create(const QString &uuid, const QString &name, const QString &desc, const QString &llmid, const QString &cmd, const QString &ext, const QString &assistantid)
{
    AppObject obj;
    obj.uuid = uuid;
    obj.name = name;
    obj.desc = desc;
    obj.llmid = llmid;
    obj.cmd = cmd;
    obj.ext = ext;
    obj.assistantid = assistantid;
    return AppTable(obj);
}

AppTable  AppTable::get(const QString &uuid)
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "SELECT * FROM app WHERE uuid=:uuid;", {{"uuid", uuid}}, {}, result, msg, "basic")) {
        if (result && result->size() > 0) {
            auto item = result->value(0);
            auto obj = AppTable::create(
                           item.value("uuid").toString(), item.value("name").toString(), item.value("desc").toString()
                           , item.value("llmid").toString(), item.value("cmd").toString(), item.value("ext").toString(), item.value("assistantid").toString());
            return obj;
        }
    } else {
        qInfo() << "sql error: " << msg;
    }
    return AppTable {};
}

int AppTable::count()
{
    return 0;
}

QList<AppTable> AppTable::getAll()
{
    QList<AppTable> appList;
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execSync("SELECT * FROM app; ", result, msg, "basic")) {
        if (result) {
            for (auto &iter : * result) {
                auto obj = AppTable::create(iter.value("uuid").toString(), iter.value("name").toString()
                                            , iter.value("desc").toString(), iter.value("llmid").toString()
                                            , iter.value("cmd").toString()
                                            , iter.value("ext").toString()
                                            , iter.value("assistantid").toString());
                appList.append(obj);
            }
        }
    } else {
        qInfo() << "sql error: " << msg;
    }
    return appList;
}

const AppObject *AppTable ::modelData() const
{
    return d.data();
}

