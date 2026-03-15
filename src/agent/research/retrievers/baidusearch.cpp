#include "baidusearch.h"
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
    QString defaultKey = "bce-v3/ALTAK-ZKb1J0NginiV0cDccUQe1/5a2a64f505bc36a5b8e37843c8a24d112fed9a88";
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
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();

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
                    QString base64Icon = urlToBase64(iconUrl);
                    if (!base64Icon.isEmpty()) {
                        normalizedResult["icon"] = base64Icon;
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

QString BaiduSearch::urlToBase64(const QString &url)
{
    if (url.isEmpty()) {
        return QString();
    }

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setInterval(5000); // 5s timeout
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();

    QString base64Str;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        base64Str = QString::fromLatin1(data.toBase64());
    } else {
        qWarning() << "Failed to download icon:" << url << reply->errorString();
    }
    reply->deleteLater();

    return base64Str;
}

} // namespace uos_ai
