#include "assistanttable.h"
#include "objects/assistantobject.h"
#include "daoclient.h"

#include <QDebug>

AssistantTable::AssistantTable()
    : d(new AssistantObject())
{

}

AssistantTable::AssistantTable(const AssistantTable &other)
    : d(other.d)
{
}

AssistantTable::AssistantTable(const AssistantObject &object)
    : d(new AssistantObject(object))
{
}

AssistantTable::~AssistantTable()
{

}

AssistantTable &AssistantTable::operator=(const AssistantTable &other)
{
    d = other.d;
    return *this;
}


QString AssistantTable::id() const
{
    return d->id;
}

void AssistantTable::setId(const QString &id)
{
    d->id = id;
}

QString AssistantTable::displayName() const
{
    return d->displayName;
}

void AssistantTable::setDisplayName(const QString &displayName)
{
    d->displayName = displayName;
}

int AssistantTable::type() const
{
    return d->type;
}

void AssistantTable::setType(int nType)
{
    d->type = nType;
}

QString AssistantTable::description() const
{
    return d->description;
}

void AssistantTable::setDescription(const QString &desc)
{
    d->description = desc;
}

bool AssistantTable::save()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
                "INSERT OR REPLACE INTO assistant "
                "(id, display_name, type, description) VALUES (:id, :display_name, :type, :description) ;"
    , {}, {{"id", id()}, {"display_name", displayName()}, {"type", type()}, {"description", description()}}, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}

bool AssistantTable::update()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "UPDATE assistant SET display_name=:display_name, type=:type, description=:description, WHERE id=:id ;", {{"id", id()}}
    , {{"id", id()}, {"display_name", displayName()}, {"type", type()}, {"description", description()}}, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}

bool AssistantTable::remove()
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "DELETE FROM assistant WHERE id=:id ;", {{"id", id()}}, {}, result, msg, "basic")) {
        return true;
    } else {
        qInfo() << "sql error: " << msg;
    }
    return false;
}


AssistantTable AssistantTable::create(const QString &id, const QString &displayName, int type, const QString &description)
{
    AssistantObject obj;
    obj.id = id;
    obj.displayName = displayName;
    obj.type = type;
    obj.description = description;
    return AssistantTable(obj);
}

AssistantTable AssistantTable::create()
{
    return AssistantTable();
}

AssistantTable AssistantTable::get(const QString &id)
{
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execBatchSync(
    "SELECT * FROM assistant WHERE id=:id;", {{"id", id}}, {}, result, msg, "basic")) {
        if (result && result->size() > 0) {
            auto item = result->value(0);
            auto obj = AssistantTable::create(
                           item.value("id").toString(), item.value("display_name").toString(), item.value("type").toInt(), item.value("description").toString());
            return obj;
        }
    } else {
        qInfo() << "sql error: " << msg;
    }
    return AssistantTable {};
}

int AssistantTable::count()
{
    return 0;
}

QList<AssistantTable> AssistantTable::getAll()
{
    QList<AssistantTable> assistantList;
    DaoResultListPtr result = nullptr;
    QString msg;
    if (DaoClient::getInstance().execSync("SELECT * FROM assistant; ", result, msg, "basic")) {
        if (result) {
            for (auto &iter : * result) {
                auto obj = AssistantTable::create(iter.value("id").toString(), iter.value("display_name").toString(), iter.value("type").toInt(), iter.value("description").toString());
                assistantList.append(obj);
            }
        }
    } else {
        qInfo() << "sql error: " << msg;
    }
    return assistantList;
}

