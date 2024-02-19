#include "aiimages.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPixmap>
#include <QFile>
#include <QBuffer>

AIImages::AIImages(const AccountProxy &account)
    : AINetWork(account)
{
    setTimeOut(60000);
}

QList<QByteArray> AIImages::parserImages(const QByteArray &data) const
{
    QList<QByteArray> imageData;
    const QJsonArray &datas = QJsonDocument::fromJson(data).object().value("data").toArray();
    for (int i = 0; i < datas.size(); i++) {
        const QByteArray &data = QByteArray::fromBase64(datas.at(i).toObject().value("b64_json").toVariant().toByteArray());
        imageData << data;
    }
    return imageData;
}

QPair<int, QString> AIImages::create(const QString &prompt, QList<QByteArray> &imageData, int n, const QString &size, const QString &format)
{
    QJsonObject dataObject;
    dataObject.insert("prompt", prompt);
    dataObject.insert("n", qBound(1, n, 4));
    dataObject.insert("size", size);
    dataObject.insert("response_format", format);

    const QPair<int, QByteArray> &resultPairs = request(dataObject, "/images/generations");
    if (resultPairs.first != 0)
        return qMakePair(resultPairs.first, resultPairs.second);

    imageData = parserImages(resultPairs.second);
    return qMakePair(0, QString());
}
