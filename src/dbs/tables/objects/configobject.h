#ifndef CONFIGOBJECT_H
#define CONFIGOBJECT_H

#include <QSharedData>

class ConfigObject : public QSharedData
{
public:
    int id = 0;

    QString name;

    int type;

    QString desc;

    QString value;

    QString tableName() const { return QStringLiteral("config"); }
};



#endif // CONFIGOBJECT_H
