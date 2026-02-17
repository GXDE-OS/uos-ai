#include "downloader.h"

#include <QEventLoop>
#include <QDir>
#include <QFile>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QLoggingCategory>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

Downloader::Downloader(const QString &directory, QObject *parent) : QObject(parent),
    m_manager(new QNetworkAccessManager(this)),
    m_downloadDirectory(directory),
    m_finished(false)
{
    connect(m_manager, &QNetworkAccessManager::finished, this, &Downloader::onDownloadFinished);
}

Downloader::~Downloader()
{
    QMutexLocker locker(&mutex);
    m_finished = true;
    foreach (QNetworkReply *reply, m_activeDownloads) {
        if (m_openFiles.contains(reply)) {
            QFile *file = m_openFiles.take(reply);
            if (file->isOpen()) {
                file->close();
            }
            file->deleteLater();
        }
        reply->deleteLater();
    }
    m_manager->deleteLater();
}

void Downloader::addDownloadTask(const QUrl &url)
{
    qCInfo(logAIGUI) << "Adding download task. URL:" << url.toString();
    QNetworkRequest request(url);

    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_manager->get(request);

    {
        QMutexLocker locker(&mutex);
        m_finished = false;
        m_activeDownloads.append(reply);
        urlToFileName.insert(url.toString(), url.fileName());
    }

    if (url.fileName().contains(".gguf"))
        connect(reply, &QNetworkReply::downloadProgress, this, &Downloader::onDownloadProgress);

    connect(reply, &QNetworkReply::readyRead, this, &Downloader::onReadyRead);
}

void Downloader::onDownloadFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
        qCWarning(logAIGUI) << "Download file failed:" << reply->url() << reply->error() << reply->errorString();

    QMutexLocker locker(&mutex);
    disconnect(reply);
    m_activeDownloads.removeAll(reply);

    if (m_openFiles.contains(reply)) {
        QFile *file = m_openFiles.take(reply);
        if (file->isOpen()) {
            file->close();
        }

        file->deleteLater();
    }

    if (m_activeDownloads.isEmpty() && !m_finished) {
        m_finished = true;
        waitCondition.wakeAll();
        emit downloadFinished();
    }

    reply->deleteLater();
}

void Downloader::onReadyRead()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    QString filename = urlToFileName.value(reply->request().url().toString(), reply->request().url().fileName());
    QString localFilePath = m_downloadDirectory + "/" + filename;
    QFile *file = m_openFiles.value(reply);
    if (!file) {
        file = new QFile(localFilePath);
        if (QFile::exists(localFilePath)) {
            QFile::remove(localFilePath);
        }
        if (!file->open(QIODevice::WriteOnly)) {
            qCWarning(logAIGUI) << "Unable to open file for writing:" << localFilePath << file->errorString();
            reply->abort();
            delete file;
            return;
        }
        m_openFiles.insert(reply, file);
    }

    file->write(reply->read(reply->bytesAvailable()));
}

void Downloader::cancelDownloads()
{
    qCInfo(logAIGUI) << "Cancelling all downloads. Directory:" << m_downloadDirectory;
    QMutexLocker locker(&mutex);
    m_finished = true;
    foreach (QNetworkReply *reply, m_activeDownloads) {
        if (m_openFiles.contains(reply)) {
            QFile *file = m_openFiles.take(reply);
            if (file->isOpen()) {
                file->close();
            }
            file->deleteLater();
        }
        reply->deleteLater();
    }

    m_activeDownloads.clear();
    m_openFiles.clear();
}

bool Downloader::checkSha256(const QString &file, const QString &sha)
{
    if (sha.isEmpty())
        return false;

    QFile binfile(file);
    if (!binfile.open(QFile::ReadOnly))
        return false;

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&binfile);

    QString fileHash = hash.result().toHex();
    return sha == fileHash;
}
