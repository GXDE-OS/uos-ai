#ifndef AIMODELS_H
#define AIMODELS_H

#include "ainetwork.h"

class AIModels : public AINetWork
{
public:
    explicit AIModels(const AccountProxy &account);

public:
    void list();

    void retrieve(const QString &model);
};

#endif // AIMODELS_H
