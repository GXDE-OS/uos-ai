#include "aiimages.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

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
    qCDebug(logLLM) << "AIImages Parsing images data, array size:" << datas.size();
    
    for (int i = 0; i < datas.size(); i++) {
        const QByteArray &data = datas.at(i).toObject().value("b64_json").toString().toUtf8();
        imageData << data;
    }
    return imageData;
}

QPair<int, QString> AIImages::create(const QString &prompt, QList<QByteArray> &imageData, int n, const QString &size, const QString &format)
{
    qCDebug(logLLM) << "AIImages Creating images with prompt:" << prompt << "n:" << n << "size:" << size << "format:" << format;

    QJsonObject dataObject;
    dataObject.insert("prompt", prompt);
    dataObject.insert("n", qBound(1, n, 4));
    dataObject.insert("size", size);
    dataObject.insert("response_format", format);

    const QPair<int, QByteArray> &resultPairs = request(dataObject, "/images/generations");
    if (resultPairs.first != 0) {
        qCWarning(logLLM) << "AIImages Image generation request failed with error:" << resultPairs.first;
        return qMakePair(resultPairs.first, resultPairs.second);
    }

    imageData = parserImages(resultPairs.second);
    return qMakePair(0, QString());
}
