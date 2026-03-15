#include "serper.h"
#include "osinfo.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

namespace uos_ai {

Serper::Serper(QObject *parent)
    : SearchEngine(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    m_apiKey = getApiKey();
    m_defaultCountry = QString("cn");
    m_defaultLanguage = QString("zh-cn");
    m_defaultTimeRange = qgetenv("SERPER_TIME_RANGE");
    m_defaultExcludeSites = getExcludeSitesFromEnv();

    if (m_apiKey.isEmpty()) {
        qWarning() << "Serper API key not found. Please set the SERPER_API_KEY environment variable. You can get a key at https://serper.dev/";
    }
}

QString Serper::getApiKey()
{
    auto defEnv = UosInfo()->pureEnvironment();
    QString defaultKey = "88fc37808768eccb529df9ed2b12ad71f2bb74c4";
    QString apiKey = defEnv.value("SERPER_API_KEY", defaultKey);

    return apiKey;
}

QStringList Serper::getExcludeSitesFromEnv()
{
//    QByteArray excludeSitesEnv = qgetenv("SERPER_EXCLUDE_SITES");
//    if (excludeSitesEnv.isEmpty()) {
//        return QStringList();
//    }

//    QStringList sites;
//    for (const QString &site : QString::fromLatin1(excludeSitesEnv).split(',')) {
//        sites.append(site.trimmed());
//    }
    return QStringList();
}

QJsonArray Serper::search(const QString &query, int maxResults)
{
    return search(query, QStringList(), QString(), QString(), QString(), QStringList(), maxResults);
}

QJsonArray Serper::search(const QString &query,
                        const QStringList &queryDomains,
                        const QString &country,
                        const QString &language,
                        const QString &timeRange,
                        const QStringList &excludeSites,
                        int maxResults)
{
    if (m_apiKey.isEmpty()) {
        qWarning() << "Serper API key is not set.";
        return QJsonArray();
    }

    QNetworkRequest request(QUrl("https://google.serper.dev/search"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("X-API-KEY", m_apiKey.toUtf8());

    QString queryWithFilters = query;
    QStringList finalExcludeSites = excludeSites.isEmpty() ? m_defaultExcludeSites : excludeSites;
    if (!finalExcludeSites.isEmpty()) {
        for (const QString &site : finalExcludeSites) {
            queryWithFilters += QString(" -site:%1").arg(site);
        }
    }

    if (!queryDomains.isEmpty()) {
        queryWithFilters += " site:" + queryDomains.join(" OR site:");
    }

    QJsonObject searchParams;
    searchParams["q"] = queryWithFilters;
    searchParams["num"] = maxResults;

    QString finalCountry = country.isEmpty() ? m_defaultCountry : country;
    if (!finalCountry.isEmpty()) {
        searchParams["gl"] = finalCountry;
    }

    QString finalLanguage = language.isEmpty() ? m_defaultLanguage : language;
    if (!finalLanguage.isEmpty()) {
        searchParams["hl"] = finalLanguage;
    }

//    QString finalTimeRange = timeRange.isEmpty() ? m_defaultTimeRange : timeRange;
//    if (!finalTimeRange.isEmpty()) {
//        searchParams["tbs"] = finalTimeRange;
//    }

    QJsonDocument doc(searchParams);
    QByteArray jsonData = doc.toJson();

    QNetworkReply *reply = m_networkManager->post(request, jsonData);

    QEventLoop loop;
    QTimer timer;
    timer.setInterval(60000);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();

    QJsonArray normalizedResults;
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Serper search error:" << reply->errorString();
    } else {
        QByteArray responseData = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);

        if (responseDoc.isObject()) {
            QJsonObject rootObject = responseDoc.object();
            QJsonArray organicResults = rootObject.value("organic").toArray();

            for (const QJsonValue &value : organicResults) {
                QJsonObject resultObj = value.toObject();
                QJsonObject normalizedResult;
                normalizedResult["title"] = resultObj.value("title");
                normalizedResult["href"] = resultObj.value("link");
                normalizedResult["body"] = resultObj.value("snippet");
                normalizedResults.append(normalizedResult);
            }
        } else {
            qWarning() << "Failed to parse JSON response from Serper.";
        }
    }

    reply->deleteLater();
    return normalizedResults;
}

} // namespace uos_ai
