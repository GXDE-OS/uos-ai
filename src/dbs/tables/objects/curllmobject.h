#pragma once
#include <QSharedData>


class CurLlmObject : public QSharedData
{
public:
    QString assistantid;

    QString llmid;

    QString tableName() const { return QStringLiteral("curllm"); }
};
