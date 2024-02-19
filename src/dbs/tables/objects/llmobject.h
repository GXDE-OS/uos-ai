#pragma once
#include <QSharedData>


class LlmObject : public QSharedData
{
public:
    QString uuid;

    QString name;

    int type;

    QString desc;

    QString account_proxy;

    QString ext;

    QString tableName() const { return QStringLiteral("llm"); }
};

