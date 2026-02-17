#include "aimodels.h"

#include <QLoggingCategory>
#include <QJsonObject>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

AIModels::AIModels(const AccountProxy &account)
    : AINetWork(account)
{

}

void AIModels::list()
{
    const QPair<int, QByteArray> &resultPairs = request(QJsonObject(), "/models");
    
    if (resultPairs.first != 0) {
        qCWarning(logLLM) << "Failed to retrieve models list, error code:" << resultPairs.first;
    } else {
        qCDebug(logLLM) << "Successfully retrieved models list" << resultPairs.second;
    }
}

void AIModels::retrieve(const QString &model)
{
    const QPair<int, QByteArray> &resultPairs = request(QJsonObject(), "/models/" + model);
    
    if (resultPairs.first != 0) {
        qCWarning(logLLM) << "Failed to retrieve model details, error code:" << resultPairs.first;
    } else {
        qCDebug(logLLM) << "Successfully retrieved model details" << resultPairs.second;
    }
}
