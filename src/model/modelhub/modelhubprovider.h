#ifndef MODELHUBPROVIDER_H
#define MODELHUBPROVIDER_H

#include "modelprovider.h"
#include "modelinfo.h"
#include "externalllm/modelhubwrapper.h"

#include <QReadWriteLock>
#include <QObject>

namespace uos_ai {

class AbstractModel;

class ModelHubProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit ModelHubProvider(QObject *parent = nullptr);
    ~ModelHubProvider() override;

    QList<ModelAccountPtr> modelList();
    AbstractModel* createModel(const ModelAccountPtr &acc) override;
    QSharedPointer<ModelhubWrapper> getWrapper(const QString &id);

private:
    QReadWriteLock mtx;
    QMap<QString, QSharedPointer<ModelhubWrapper>> wrapper;
};

} // namespace uos_ai

#endif // MODELHUBPROVIDER_H
