#ifndef VOLCANOENGINE_H
#define VOLCANOENGINE_H

#include "searchengine.h"
#include <QNetworkAccessManager>

namespace uos_ai {

class VolcanoEngine : public SearchEngine
{
    Q_OBJECT

public:
    explicit VolcanoEngine(QObject *parent = nullptr);

    QJsonArray search(const QString &query, int maxResults = 5) override;
protected:
    QString downloadAndSaveIcon(const QString &iconUrl, const QString &pageUrl);
private:
    QString getApiKey();

    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
};

} // namespace uos_ai

#endif // VOLCANOENGINE_H
