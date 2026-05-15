#ifndef COZEPROVIDER_H
#define COZEPROVIDER_H

#include "modelprovider.h"
#include "modelinfo.h"

#include <QObject>

namespace uos_ai {

class AbstractModel;

class CozeProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit CozeProvider(QObject *parent = nullptr);
    ~CozeProvider() override;

    AbstractModel* createModel(const ModelAccountPtr &acc) override;

    static QString host();
};

} // namespace uos_ai

#endif // COZEPROVIDER_H
