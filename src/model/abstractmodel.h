#ifndef ABSTRACTMODEL_H
#define ABSTRACTMODEL_H

#include "global_define.h"
#include "modelinfo.h"

#include <QObject>

namespace uos_ai {

class AbstractModel : public QObject
{
    Q_OBJECT
public:
    explicit AbstractModel(QObject *parent = nullptr);
    virtual ~AbstractModel();

    virtual void setAccount(const ModelAccountPtr &accountPtr);
    virtual ModelAccountPtr account() const;

    virtual void setParameters(const QVariantHash &params);
    virtual QVariantHash parameters() const;
    virtual void updateParameters(const QVariantHash &params);

    virtual QVariantHash lastError() const;
    virtual void setError(const QVariantHash &err);
protected:
    ModelAccountPtr m_account;
    QVariantHash m_parameters;
    QVariantHash m_error;
};

} // namespace uos_ai

#endif // MODEL_ABSTRACTMODEL_H
