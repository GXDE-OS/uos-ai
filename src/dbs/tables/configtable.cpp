#include "configtable.h"
#include "objects/configobject.h"
#include "daoclient.h"

#include <QDebug>

ConfigTable::ConfigTable()
    : DbBase()
    , d(new ConfigObject())
{

}

ConfigTable::ConfigTable(const ConfigTable &other)
    : d(other.d)
{

}

ConfigTable::ConfigTable(const ConfigObject &object)
    : d(new ConfigObject(object))
{

}

ConfigTable::~ConfigTable()
{
}

ConfigTable  &ConfigTable::operator=(const ConfigTable &other)
{
    d = other.d;
    return *this;
}


const ConfigObject *ConfigTable::modelData() const
{
    return d.data();
}

int ConfigTable::id()
{
    return d->id;
}

QString ConfigTable::name() const
{
    return d->name;
}

void ConfigTable::setName(const QString &name)
{
    d->name = name;
}

int ConfigTable::type() const
{
    return d->type;
}

void ConfigTable::setType(const int &type)
{
    d->type = type;
}

QString ConfigTable::desc() const
{
    return d->desc;
}

void ConfigTable::setDesc(const QString &desc)
{
    d->desc = desc;
}

QString ConfigTable::value() const
{
    return d->value;
}

void ConfigTable::setValue(const QString &value)
{
    d->value = value;
}

bool ConfigTable::save()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
                "INSERT INTO config (name, type, desc, value) VALUES (:name, :type, :desc, :value) ;", {}
    , {{"name", name()}, {"type", type()}, {"desc", desc()}, {"value", value()}}, result, msg, "basic")) {
        return true;
    } else {
        qWarning() << "sql error: " << msg;
    }
    return false;
}

bool ConfigTable::update()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "UPDATE config SET name=:name, desc=:desc, value=:value WHERE type=:type ;", {{"type", type()}}
    , {{"name", name()}, {"desc", desc()}, {"value", value()}}, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}

bool ConfigTable::remove()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "DELETE FROM config WHERE type=:type;", {{"type", type()}}, {}, result, msg, "basic")) {
        return true;
    } else {
        qWarning() << "sql error: " << msg;
    }
    return false;
}

ConfigTable  ConfigTable::create()
{
    return ConfigTable();
}

ConfigTable  ConfigTable::create(int id, const QString &name, int type, const QString &desc, const QString &value)
{
    ConfigObject obj;
    obj.id = id;
    obj.name = name;
    obj.type = type;
    obj.desc = desc;
    obj.value = value;
    return ConfigTable(obj);
}

ConfigTable  ConfigTable::get(int type)
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "SELECT * FROM config WHERE type=:type;", {{"type", type}}, {}, result, msg, "basic")) {
        if (result && result->size() > 0) {
            auto item = result->value(0);
            auto obj = ConfigTable::create(
                           item.value("id").toInt(), item.value("name").toString(), item.value("type").toInt(), item.value("desc").toString()
                           , item.value("value").toString());
            return obj;
        }
    } else {
        qWarning() << "sql error: " << msg;
    }
    return ConfigTable {};
}

QList<ConfigTable> ConfigTable::getAll()
{

    QList<ConfigTable> confgList;
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execSync("SELECT * FROM config; ", result, msg, "basic")) {
        if (result) {
            for (auto &item : * result) {
                auto obj = ConfigTable::create(
                               item.value("id").toInt(), item.value("name").toString(), item.value("type").toInt(), item.value("desc").toString()
                               , item.value("value").toString());
                confgList.append(obj);
            }
        }
    } else {
        qWarning() << "sql error: " << msg;
    }
    return confgList;
}

int ConfigTable::count()
{
    return 0;
}
























