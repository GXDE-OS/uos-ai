#include "volcanoengine.h"
#include "osinfo.h"
#include "../tools/iconstore.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QUrlQuery>

namespace uos_ai {

VolcanoEngine::VolcanoEngine(QObject *parent)
    : SearchEngine(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    m_apiKey = getApiKey();

    if (m_apiKey.isEmpty()) {
        qWarning() << "Volcano Engine API key not found. Please set the VOLCANO_ENGINE_API_KEY environment variable.";
    }
}

QString VolcanoEngine::getApiKey()
{
    auto defEnv = UosInfo()->pureEnvironment();
    QString defaultKey = "8prrmRMvOSSKoUslywAIuLFkiIpj004e";
    QString apiKey = defEnv.value("VOLCANO_ENGINE_API_KEY", defaultKey);

    return apiKey;
}

QJsonArray VolcanoEngine::search(const QString &query, int maxResults)
{
    if (m_apiKey.isEmpty()) {
        qWarning() << "Volcano Engine API key is not set.";
        return QJsonArray();
    }

    QUrl url("https://open.feedcoopapi.com/search_api/web_search");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    QJsonObject filter;
    filter["NeedContent"] = true;
    filter["NeedUrl"] = true;

    QJsonObject payload;
    payload["Query"] = query;
    payload["SearchType"] = "web";
    payload["Count"] = maxResults;
    payload["Filter"] = filter;
    payload["NeedSummary"] = true;

    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson();

    QNetworkReply *reply = m_networkManager->post(request, jsonData);

    QEventLoop loop;
    QTimer timer;
    timer.setInterval(60000);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    connect(this, &SearchEngine::requestAbort, reply, &QNetworkReply::abort);
    timer.start();
    loop.exec();
    timer.stop();

    QJsonArray normalizedResults;
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Volcano Engine search error:" << reply->errorString();
    } else {
        QByteArray responseData = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);

        if (responseDoc.isObject()) {
            QJsonObject rootObject = responseDoc.object();
            QJsonObject resultObj = rootObject.value("Result").toObject();
            QJsonArray webResults = resultObj.value("WebResults").toArray();

            for (const QJsonValue &value : webResults) {
                QJsonObject resultItem = value.toObject();
                QJsonObject normalizedResult;
                normalizedResult["title"] = resultItem.value("Title");
                normalizedResult["href"] = resultItem.value("Url");
                normalizedResult["snippet"] = resultItem.value("Snippet");
                normalizedResult["website"] = resultItem.value("SiteName");
                normalizedResult["summary"] = resultItem.value("Summary");
                //normalizedResult["content"] = resultItem.value("Content");

                if (resultItem.contains("LogoUrl")) {
                    QString iconUrl = resultItem.value("LogoUrl").toString();
                    QString pageUrl = resultItem.value("Url").toString();
                    QString iconKey = downloadAndSaveIcon(iconUrl, pageUrl);
                    if (!iconKey.isEmpty()) {
                        normalizedResult["icon"] = iconKey;
                    }
                }

                normalizedResults.append(normalizedResult);
            }
        } else {
            qWarning() << "Failed to parse JSON response from Volcano Engine.";
        }
    }

    reply->deleteLater();
    return normalizedResults;
}

QString VolcanoEngine::downloadAndSaveIcon(const QString &iconUrl, const QString &pageUrl)
{
    if (iconUrl.isEmpty())
        return QString();

    // Derive domain key from page URL (or icon URL as fallback)
    QString key = IconStore::domainKey(pageUrl);
    if (key.isEmpty())
        key = IconStore::domainKey(iconUrl);
    if (key.isEmpty())
        return QString();

    // Skip download if icon already cached
    if (IconStore::instance()->exists(key))
        return key;

    QNetworkRequest request(iconUrl);
    QNetworkReply *reply = m_networkManager->get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setInterval(5000); // 5s timeout
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    timer.start();
    loop.exec();
    timer.stop();

    QString resultKey;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        resultKey = IconStore::instance()->saveFromData(key, data);
    } else {
        qWarning() << "Failed to download icon:" << iconUrl << reply->errorString();
    }
    reply->deleteLater();

    return resultKey;
}

} // namespace uos_ai
