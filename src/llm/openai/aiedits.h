#ifndef AIEDITS_H
#define AIEDITS_H

#include "ainetwork.h"

class AIEdits : public AINetWork
{
public:
    explicit AIEdits(const AccountProxy &account);

    QPair<int, QString> create(const QString &modelId, QString input, QString instruction, uint16_t n = 1);
};

#endif // EDITS_H
