#ifndef DOWNLOADER_H
#define DOWNLOADER_H
#include "uosai_global.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>

namespace uos_ai {

class Downloader : public QObject
{
    Q_OBJECT

public:
    explicit Downloader(const QString &directory, QObject *parent = nullptr);
    ~Downloader();

    void addDownloadTask(const QUrl &url);
    void cancelDownloads();
    bool isFinished() { return m_finished; }
    static bool checkSha256(const QString &file, const QString &sha);
signals:
    void downloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void onDownloadFinished(QNetworkReply *reply);
    void onReadyRead();

private:
    QNetworkAccessManager *m_manager;
    QString m_downloadDirectory;
    QList<QNetworkReply*> m_activeDownloads;
    QHash<QString, QString> urlToFileName;
    bool m_finished;

    QMutex mutex;
    QWaitCondition waitCondition;
    QHash<QNetworkReply*, QFile*> m_openFiles;
};
}

#endif // DOWNLOADER_H
