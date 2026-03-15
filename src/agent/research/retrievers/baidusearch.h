#ifndef BAIDUSEARCH_H
#define BAIDUSEARCH_H

#include "searchengine.h"
#include <QNetworkAccessManager>

namespace uos_ai {

class BaiduSearch : public SearchEngine
{
    Q_OBJECT

public:
    explicit BaiduSearch(QObject *parent = nullptr);

    QJsonArray search(const QString &query, int maxResults = 5) override;

private:
    QString getApiKey();

    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;

    QString urlToBase64(const QString &url);
};

} // namespace uos_ai

#endif // BAIDUSEARCH_H
