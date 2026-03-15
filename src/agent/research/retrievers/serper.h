#ifndef SERPER_H
#define SERPER_H

#include "searchengine.h"
#include <QNetworkAccessManager>

namespace uos_ai {

class Serper : public SearchEngine
{
    Q_OBJECT

public:
    explicit Serper(QObject *parent = nullptr);

    QJsonArray search(const QString &query, int maxResults = 5) override;

    QJsonArray search(const QString &query,
                      const QStringList &queryDomains,
                      const QString &country,
                      const QString &language,
                      const QString &timeRange,
                      const QStringList &excludeSites,
                      int maxResults);

private:
    QString getApiKey();
    QStringList getExcludeSitesFromEnv();

    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    QString m_defaultCountry;
    QString m_defaultLanguage;
    QString m_defaultTimeRange;
    QStringList m_defaultExcludeSites;
};

} // namespace uos_ai

#endif // SERPER_H
