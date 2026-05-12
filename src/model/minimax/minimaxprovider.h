#ifndef MINIMAXPROVIDER_H
#define MINIMAXPROVIDER_H

#include "modelprovider.h"
#include "modelinfo.h"

#include <QObject>

namespace uos_ai {

class AbstractModel;

class MiniMaxProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit MiniMaxProvider(QObject *parent = nullptr);
    ~MiniMaxProvider() override;

    AbstractModel* createModel(const ModelAccountPtr &acc) override;

    static QString host();
};

} // namespace uos_ai

#endif // MINIMAXPROVIDER_H
