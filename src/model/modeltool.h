#ifndef MODELTOOL_H
#define MODELTOOL_H

#include <QString>
#include <QList>
#include <QVariantHash>

namespace uos_ai {
// OpenAI tool schema

struct ModelToolProperty
{
    QString name;
    QString type;
    QString description;
    QStringList enums;
};

struct ModelTool {
    QString name;
    QString description;
    // parameters
    QList<ModelToolProperty> properties; 
    QStringList required;

    static ModelTool fromOai(const QJsonObject &);
};

struct ModelToolCall
{
    QString id;
    QString name;
    QVariantHash arguments;

    inline bool isValid() {
        return !id.isEmpty() && !name.isEmpty();
    }
};

using ModelToolCallList = QList<uos_ai::ModelToolCall>;
using ModelToolList = QList<uos_ai::ModelTool>;
}

Q_DECLARE_METATYPE(uos_ai::ModelToolCall)
Q_DECLARE_METATYPE(uos_ai::ModelToolCallList)
Q_DECLARE_METATYPE(uos_ai::ModelToolList)

#endif // MODELTOOL_H
