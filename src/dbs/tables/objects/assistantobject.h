#pragma once
#include <QSharedData>


class AssistantObject : public QSharedData
{
public:
    QString id;

    QString displayName;

    int type;

    QString description;

    QString tableName() const { return QStringLiteral("assistant"); }
};

