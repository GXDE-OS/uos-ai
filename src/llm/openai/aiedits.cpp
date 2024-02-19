#include "aiedits.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

AIEdits::AIEdits(const AccountProxy &account)
    : AINetWork(account)
{

}

QPair<int, QString> AIEdits::create(const QString &modelId, QString input, QString instruction, uint16_t n)
{
    QJsonObject dataObject;
    dataObject.insert("model", modelId);
    dataObject.insert("input", input);
    dataObject.insert("instruction", instruction);
    dataObject.insert("n", n);

    const QPair<int, QByteArray> &resultPairs = request(dataObject, "/edits");

    QString resultData;
    const QJsonObject &obj = QJsonDocument::fromJson(resultPairs.second).object();
    const QJsonArray &array =  obj.value("choices").toArray();
    for (int i = 0; i < array.size(); i++) {
        const QJsonObject &choicesObj = array.at(i).toObject();
        resultData = choicesObj.value("text").toString().trimmed();
    }

    return qMakePair(resultPairs.first, resultData);
}
