#pragma once
#include <QSharedData>

class AppObject : public QSharedData
{
public:
    QString uuid;

    QString name;

    QString desc;

    QString llmid;

    QString cmd;

    QString ext;

    QString tableName() const { return QStringLiteral("app"); }
};

