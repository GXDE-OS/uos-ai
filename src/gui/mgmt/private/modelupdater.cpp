#include "modelupdater.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QLoggingCategory>

using namespace uos_ai;
Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

ModelUpdater::ModelUpdater(const QString &baseUrl, const QString &dirPath,
                           const QString &appName, const QString &modelName, QObject *parent)
    : QObject(parent), m_appName(appName), m_modelName(modelName), m_baseUrl(baseUrl), m_installPath(dirPath)
{
}

void ModelUpdater::check()
{
    if (m_isRequsting) {
        qCDebug(logAIGUI) << "Check already in progress, skipping.";
        return;
    }

    m_isRequsting = true;
    QNetworkRequest request(QUrl(m_baseUrl + "/" + m_appName + "/resolve/master/sha256"));
    qCInfo(logAIGUI) << "Sending update check request. URL:" << request.url();
    QNetworkReply *reply = m_manager.get(request);
    connect(reply, &QNetworkReply::readyRead, this, &ModelUpdater::onReadyRead);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onFinished(reply);
        reply->deleteLater();
    });
}

void ModelUpdater::saveNewHash()
{
    if (m_remote.isEmpty()) {
        qCWarning(logAIGUI) << "Remote data is empty, cannot save new hash.";
        return;
    }
    QCryptographicHash hasher(QCryptographicHash::Sha256);
    hasher.addData(m_remote);
    QByteArray hashValue = hasher.result().toHex();

    QFile file(m_installPath + "/" + m_modelName + "/gguf/.new_sha256");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(hashValue);
        file.close();
        qCInfo(logAIGUI) << "New hash saved to file:" << file.fileName();
    } else {
        qCWarning(logAIGUI) << "Failed to open file for writing new hash:" << file.fileName();
    }
}

QPair<bool, bool> ModelUpdater::compareSha()
{
    QCryptographicHash localHasher(QCryptographicHash::Sha256);
    QFile localFile(m_installPath + "/" + m_modelName + "/gguf/sha256");
    if (localFile.open(QIODevice::ReadOnly)) {
        localHasher.addData(&localFile);
        localFile.close();
        qCDebug(logAIGUI) << "Read local sha256 file successfully.";
    } else {
        qCWarning(logAIGUI) << "Failed to open local sha256 file:" << localFile.fileName();
    }

    QByteArray localHash = localHasher.result().toHex();

    QCryptographicHash remoteHasher(QCryptographicHash::Sha256);
    remoteHasher.addData(m_remote);
    QByteArray remoteHash = remoteHasher.result().toHex();

    bool localCmpRemote = (localHash != remoteHash);
    qCDebug(logAIGUI) << "Compare local and remote hash. Local:" << localHash << ", Remote:" << remoteHash << ", Different:" << localCmpRemote;

    bool newlocalCmpRemote = false;
    if (localCmpRemote) {
        QFile newLocalFile(m_installPath + "/" + m_modelName + "/gguf/.new_sha256");
        QByteArray newLocalHash = "";
        if (newLocalFile.open(QIODevice::ReadOnly)) {
            newLocalHash = newLocalFile.readAll().trimmed();
            newLocalFile.close();
            qCDebug(logAIGUI) << "Read .new_sha256 file successfully.";
        } else {
            qCWarning(logAIGUI) << "Failed to open .new_sha256 file:" << newLocalFile.fileName();
        }

        newlocalCmpRemote = (newLocalHash != remoteHash);
        qCDebug(logAIGUI) << "Compare .new_sha256 and remote hash. .new_sha256:" << newLocalHash << ", Remote:" << remoteHash << ", Different:" << newlocalCmpRemote;
    }

    return QPair<bool, bool>(localCmpRemote, localCmpRemote && newlocalCmpRemote);
}

void ModelUpdater::onReadyRead()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        m_remote.clear();
        m_remote.append(reply->readAll());
        qCDebug(logAIGUI) << "Received remote data, size:" << m_remote.size();
    } else {
        qCWarning(logAIGUI) << "onReadyRead sender is not QNetworkReply.";
    }
}

void ModelUpdater::onFinished(QNetworkReply *reply)
{
    m_isRequsting = false;
    if (reply->error() == QNetworkReply::NoError) {
        qCInfo(logAIGUI) << "Update check finished successfully.";
        emit canUpdate(compareSha());
    } else {
        qCWarning(logAIGUI) << "Update check failed. Error:" << reply->errorString();
    }
}
