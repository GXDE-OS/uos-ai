#include "aimodels.h"

#include <QJsonObject>

AIModels::AIModels(const AccountProxy &account)
    : AINetWork(account)
{

}

void AIModels::list()
{
    const QPair<int, QByteArray> &resultPairs = request(QJsonObject(), "/models");
    qInfo() << resultPairs.second.data();
}

void AIModels::retrieve(const QString &model)
{
    const QPair<int, QByteArray> &resultPairs = request(QJsonObject(), "/models/" + model);
    qInfo() << resultPairs.second.data();
}
