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

    /**
     * 下载 favicon 并保存到 IconStore，返回图标键名。
     * @param iconUrl  favicon URL
     * @param pageUrl  所属页面 URL（用于提取域名作为键）
     */
    QString downloadAndSaveIcon(const QString &iconUrl, const QString &pageUrl);

    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
};

} // namespace uos_ai

#endif // BAIDUSEARCH_H
