#include "baidusearch.h"
#include "../tools/iconstore.h"
#include "osinfo.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

namespace uos_ai {

BaiduSearch::BaiduSearch(QObject *parent)
    : SearchEngine(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    m_apiKey = getApiKey();

    if (m_apiKey.isEmpty()) {
        qWarning() << "Baidu Search API key not found. Please set the BAIDU_SEARCH_API_KEY environment variable.";
    }
}

QString BaiduSearch::getApiKey()
{
    auto defEnv = UosInfo()->pureEnvironment();
    // Using the key provided in the python example as default
    QString defaultKey = "bce-v3/ALTAK-Xr4uS1tkM500HqkkbrXnZ/3cb370e143294753a0ddbee89840e51e76dce1d9";
    QString apiKey = defEnv.value("BAIDU_SEARCH_API_KEY", defaultKey);

    return apiKey;
}

QJsonArray BaiduSearch::search(const QString &query, int maxResults)
{
    if (m_apiKey.isEmpty()) {
        qWarning() << "Baidu Search API key is not set.";
        return QJsonArray();
    }

    QNetworkRequest request(QUrl("https://qianfan.baidubce.com/v2/ai_search/web_search"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString bearerToken = m_apiKey.startsWith("Bearer ") ? m_apiKey : "Bearer " + m_apiKey;
    request.setRawHeader("Authorization", bearerToken.toUtf8());

    QJsonObject message;
    message["role"] = "user";
    message["content"] = query;

    QJsonArray messages;
    messages.append(message);

    QJsonObject resourceTypeFilter;
    resourceTypeFilter["type"] = "web";
    resourceTypeFilter["top_k"] = maxResults;
    
    QJsonArray resourceTypeFilters;
    resourceTypeFilters.append(resourceTypeFilter);

    QJsonObject payload;
    payload["messages"] = messages;
    payload["edition"] = "standard";
    payload["resource_type_filter"] = resourceTypeFilters;
    
    QJsonArray blockWebsites;
    blockWebsites.append("baijiahao.baidu.com");
    payload["block_websites"] = blockWebsites;
    
    payload["search_recency_filter"] = "year";
    payload["search_source"] = "baidu_search_v2";

    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson();

    QNetworkReply *reply = m_networkManager->post(request, jsonData);

    QEventLoop loop;
    QTimer timer;
    timer.setInterval(60000); // 60s timeout
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    connect(this, &SearchEngine::requestAbort, reply, &QNetworkReply::abort);
    timer.start();
    loop.exec();
    timer.stop();

    QJsonArray normalizedResults;
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Baidu search error:" << reply->errorString();
    } else {
        QByteArray responseData = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);

        if (responseDoc.isObject()) {
            QJsonObject rootObject = responseDoc.object();
            
            QJsonArray results;
            if (rootObject.contains("references")) {
                results = rootObject.value("references").toArray();
            }

            for (const QJsonValue &value : results) {
                QJsonObject resultObj = value.toObject();
                QJsonObject normalizedResult;
                
                if (resultObj.contains("title"))
                    normalizedResult["title"] = resultObj.value("title");
                if (resultObj.contains("url"))
                    normalizedResult["href"] = resultObj.value("url");
                if (resultObj.contains("snippet"))
                    normalizedResult["snippet"] = resultObj.value("snippet");
                if (resultObj.contains("website")) {
                    QString website = resultObj.value("website").toString();
                    if (website == "无") {
                        normalizedResult["website"] = "";
                    } else {
                        normalizedResult["website"] = website;
                    }
                }
                if (resultObj.contains("icon")) {
                    QString iconUrl = resultObj.value("icon").toString();
                    QString pageUrl = resultObj.value("url").toString();
                    QString iconKey = downloadAndSaveIcon(iconUrl, pageUrl);
                    if (!iconKey.isEmpty()) {
                        normalizedResult["icon"] = iconKey;
                    }
                }

                if (!normalizedResult.isEmpty()) {
                     normalizedResults.append(normalizedResult);
                }
            }            
        } else {
            qWarning() << "Failed to parse JSON response from Baidu Search.";
        }
    }

    reply->deleteLater();
    return normalizedResults;
}

QString BaiduSearch::downloadAndSaveIcon(const QString &iconUrl, const QString &pageUrl)
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
