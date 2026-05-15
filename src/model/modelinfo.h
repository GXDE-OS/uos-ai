#ifndef MODELINFO_H
#define MODELINFO_H

#include "global_define.h"

#include <QString>
#include <QVariantHash>
#include <QSharedData>

namespace uos_ai {

struct ModelInfo
{
    QString id;
    QString name;
    ModelArch arch;
    ModelAbilities ability;
    QString modelId;
    QString icon;
};

struct ProviderAccount
{
    QString id;
    QString name;
    QVariantHash auth;
    QString provider;

    QVariantHash additional;
};

class ModelAccount : public QSharedData
{
public:
    QString id;
    ProviderAccount account;

    ModelInfo model;
    QVariantHash params;

    QString network; // online, local, private

    ModelAccount();
    ModelAccount(const ModelAccount &other);
};

using ModelAccountPtr = QSharedDataPointer<ModelAccount>;
}


#endif // MODELINFO_H
