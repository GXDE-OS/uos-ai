#include "wxqftext2image.h"

#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonArray>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

WXQFText2Image::WXQFText2Image(const AccountProxy &account)
    : WXQFNetWork(account)
{
    setTimeOut(60000);
}

QPair<int, QString> WXQFText2Image::create(const QString &prompt, QList<QByteArray> &imageData, int number)
{
    qCDebug(logLLM) << "WXQF Creating image with prompt size" << prompt.size();
    
    QJsonObject dataObject;
    dataObject.insert("prompt", prompt);
    dataObject.insert("n", qBound(1, number, 4));
    dataObject.insert("size", "768x768");

    if (prompt.length() > 256) {
        qCWarning(logLLM) << "WXQF Image generation failed: prompt exceeds 256 characters limit";
        return qMakePair(AIServer::GenerateImagePromptExceedError,
                         QCoreApplication::translate("WXQFText2Image", "The image description exceeds the 256 character limit"));
    }

    const QString path = "https://aip.baidubce.com/rpc/2.0/ai_custom/v1/wenxinworkshop/text2image/sd_xl";
    qCDebug(logLLM) << "WXQF Sending request to:" << path;
    const QPair<int, QByteArray> &resultPairs = request(dataObject, path);

    if (resultPairs.first != 0) {
        qCCritical(logLLM) << "WXQF Request failed with error code:" << resultPairs.first;
        return qMakePair(resultPairs.first, resultPairs.second);
    }

    const QJsonObject &response = QJsonDocument::fromJson(resultPairs.second).object();
    const QJsonArray &imageLst = response.value("data").toArray();
    qCInfo(logLLM) << "WXQF Received" << imageLst.size() << "images from API";
    
    for (int i = 0; i < imageLst.size(); i++) {
        imageData << imageLst.at(i).toObject().value("b64_image").toString().toUtf8();
    }

    return qMakePair(0, QString());
}
