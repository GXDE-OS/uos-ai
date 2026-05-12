#ifndef MODEL_MODELPROVIDER_H
#define MODEL_MODELPROVIDER_H

#include "modelinfo.h"

#include <QObject>
#include <QList>

namespace uos_ai {

class AbstractModel;

class ModelProvider : public QObject
{
    Q_OBJECT
public:
    explicit ModelProvider(QObject *parent = nullptr);
    virtual ~ModelProvider();

    virtual AbstractModel* createModel(const ModelAccountPtr &acc) = 0;
};

} // namespace uos_ai

#endif // MODEL_MODELPROVIDER_H
