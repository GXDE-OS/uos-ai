#ifndef EMBEDDINGSERVER_H
#define EMBEDDINGSERVER_H

#include "serverdefs.h"

#include <QObject>
#include <QDBusInterface>

class EmbeddingServer : public QObject
{
    Q_OBJECT

public:
    static EmbeddingServer &getInstance();
    virtual ~EmbeddingServer();

    bool createVectorIndex(const QStringList &files);
    bool deleteVectorIndex(const QStringList &files);

    QStringList searchVecor(const QString &query, int topK, AssistantType type);
    QStringList getDocFiles();
    QStringList getDocContent();
    void saveAllIndex();

private:
    explicit EmbeddingServer(QObject *parent = nullptr);
    QDBusInterface *embedInterface = nullptr;

    QString appid;

signals:
    void addToServerStatusChanged(const QStringList &files, int status);
    void indexDeleted(const QStringList &files);

public slots:
    void onEmbedStatusChanged(const QString &app, const QStringList &files, int status);
    void onIndexDeleteFinished(const QString &app, const QStringList &files);
};

#endif // EMBEDDINGSERVER_H
