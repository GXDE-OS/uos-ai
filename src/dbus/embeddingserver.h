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
    QString embeddingSearch(const QString &query, int topK, AssistantType type);
    QVector<QPair<int, QString>> getDocFiles();
    QStringList getDocContent();
    void saveAllIndex();

    // Embedding Text
    QVector<QVector<float>> embeddingTexts(const QStringList &texts);

private:
    explicit EmbeddingServer(QObject *parent = nullptr);
    QDBusInterface *embedInterface = nullptr;
    QJsonObject getDocList();

    QString appid;

signals:
    void addToServerStatusChanged(const QStringList &files, int status);
    void indexDeleted(const QStringList &files);

public slots:
    void onEmbedStatusChanged(const QString &app, const QStringList &files, int status);
    void onIndexDeleteFinished(const QString &app, const QStringList &files);
};

#endif // EMBEDDINGSERVER_H
