#ifndef BAILIANPROVIDER_H
#define BAILIANPROVIDER_H

#include "modelprovider.h"
#include "modelinfo.h"

#include <QObject>

namespace uos_ai {

class AbstractModel;

class BailianProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit BailianProvider(QObject *parent = nullptr);
    ~BailianProvider() override;

    AbstractModel* createModel(const ModelAccountPtr &acc) override;

    static QString host();
};

}

#endif // BAILIANPROVIDER_H
