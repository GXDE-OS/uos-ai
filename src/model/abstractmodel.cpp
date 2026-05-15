#include "abstractmodel.h"

namespace uos_ai {

AbstractModel::AbstractModel(QObject *parent) : QObject(parent)
{
}

AbstractModel::~AbstractModel()
{
}

void AbstractModel::setAccount(const ModelAccountPtr &accountPtr)
{
    m_account = accountPtr;
}

ModelAccountPtr AbstractModel::account() const
{
    return m_account;
}

void AbstractModel::setParameters(const QVariantHash &params)
{
    m_parameters = params;
}

QVariantHash AbstractModel::parameters() const
{
    return m_parameters;
}

void AbstractModel::updateParameters(const QVariantHash &params)
{
    for (auto it = params.begin(); it != params.end(); ++it)
        m_parameters.insert(it.key(), it.value());
}

QVariantHash AbstractModel::lastError() const
{
    return m_error;;
}

void AbstractModel::setError(const QVariantHash &err)
{
    m_error = err;
}

} // namespace uos_ai
