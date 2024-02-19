#include "wxqftext2image.h"

#include <QJsonDocument>
#include <QJsonArray>

WXQFText2Image::WXQFText2Image(const AccountProxy &account)
    : WXQFNetWork(account)
{
    setTimeOut(60000);
}

QPair<int, QString> WXQFText2Image::create(const QString &prompt, QList<QByteArray> &imageData, int number)
{
    QJsonObject dataObject;
    dataObject.insert("prompt", prompt);
    dataObject.insert("n", qBound(1, number, 4));
    dataObject.insert("size", "768x768");

    const QString path = "https://aip.baidubce.com/rpc/2.0/ai_custom/v1/wenxinworkshop/text2image/sd_xl";
    const QPair<int, QByteArray> &resultPairs = request(dataObject, path);

    if (resultPairs.first != 0)
        return qMakePair(resultPairs.first, resultPairs.second);

    const QJsonObject &response = QJsonDocument::fromJson(resultPairs.second).object();
    const QJsonArray &imageLst = response.value("data").toArray();
    for (int i = 0; i < imageLst.size(); i++) {
        imageData << QByteArray::fromBase64(imageLst.at(i).toObject().value("b64_image").toVariant().toByteArray());
    }

    return qMakePair(0, QString());
}
