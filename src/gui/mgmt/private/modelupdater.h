#ifndef MODELUPDATER_H
#define MODELUPDATER_H

#include <QObject>
#include <QVariantHash>
#include <QNetworkAccessManager>

namespace uos_ai {

class ModelUpdater : public QObject
{
    Q_OBJECT
public:
    explicit ModelUpdater(const QString &baseUrl, const QString &dirPath,
                          const QString &appName, const QString &modelName, QObject *parent = nullptr);
    void check();
    void saveNewHash();

signals:
    void canUpdate(QPair<bool, bool>);

protected:
    QPair<bool, bool> compareSha();

private slots:
    void onReadyRead();
    void onFinished(QNetworkReply *reply);

private:
    QString m_appName;
    QString m_modelName;
    QString m_baseUrl;
    QString m_installPath;
    QNetworkAccessManager m_manager;
    QByteArray m_remote;
    qint64 m_lastCheck = 0;
    bool m_isRequsting = false;
};

}
#endif // MODELUPDATER_H
