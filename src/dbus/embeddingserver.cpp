#include "embeddingserver.h"

#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusConnection>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

static constexpr char kSystemAssistantEmbedding[] { "SystemAssistant" };
static constexpr char kPersonlAssistantEmbedding[] { "uos-ai-assistant" };

#define NM_SERVICE      "org.deepin.ai.daemon.VectorIndex"
#define NM_PATH         "/org/deepin/ai/daemon/VectorIndex"
#define NM_INTERFACE    "org.deepin.ai.daemon.VectorIndex"

EmbeddingServer::EmbeddingServer(QObject *parent) : QObject(parent)
{
    embedInterface = new QDBusInterface(NM_SERVICE,
                                   NM_PATH,
                                   NM_INTERFACE,
                                   QDBusConnection::sessionBus());

    QDBusConnection::sessionBus().connect(NM_SERVICE,
                                         NM_PATH,
                                         NM_INTERFACE,
                                         "IndexStatus",
                                         this, SLOT(onEmbedStatusChanged(const QString &, const QStringList &, int)));
    QDBusConnection::sessionBus().connect(NM_SERVICE,
                                         NM_PATH,
                                         NM_INTERFACE,
                                         "IndexDeleted",
                                         this, SIGNAL(onIndexDeleteFinished(const QString &, const QStringList &)));

    appid = kPersonlAssistantEmbedding;
}

QJsonObject EmbeddingServer::getDocList()
{    
    QDBusPendingCall pendingCall = embedInterface->asyncCall("DocFiles", QVariant(appid));
    pendingCall.waitForFinished();
    QDBusMessage reply = pendingCall.reply();
    QString result = reply.arguments().at(0).toString();
    return QJsonDocument::fromJson(result.toUtf8()).object();
}

EmbeddingServer::~EmbeddingServer()
{
    QDBusConnection::sessionBus().disconnect(NM_SERVICE,
                                            NM_PATH,
                                            NM_INTERFACE,
                                            "IndexStatus",
                                            this, SLOT(onEmbedStatusChanged(QString, QStringList, int)));
    QDBusConnection::sessionBus().disconnect(NM_SERVICE,
                                         NM_PATH,
                                         NM_INTERFACE,
                                         "IndexDeleted",
                                         this, SIGNAL(onIndexDeleteFinished(const QString &, const QStringList &)));
}

bool EmbeddingServer::createVectorIndex(const QStringList &files)
{
    QDBusPendingCall pendingCall = embedInterface->asyncCall("Create", QVariant(appid), QVariant(files));
    pendingCall.waitForFinished();
    QDBusMessage reply = pendingCall.reply();
    bool result = reply.arguments().at(0).toBool();
    
    if (pendingCall.isValid()) {
        qCDebug(logDBus) << "Vector index creation succeeded";
    } else {
        qCWarning(logDBus) << "Vector index creation failed:" << reply.errorMessage();
    }
    return result;
}

bool EmbeddingServer::deleteVectorIndex(const QStringList &files)
{
    qCDebug(logDBus) << "Deleting vector index for files:" << files;
    
    QDBusPendingCall pendingCall = embedInterface->asyncCall("Delete", QVariant(appid), QVariant(files));
    pendingCall.waitForFinished();
    QDBusMessage reply = pendingCall.reply();
    bool result = reply.arguments().at(0).toBool();
    
    if (pendingCall.isValid()) {
        qCDebug(logDBus) << "Vector index deletion succeeded";
    } else {
        qCWarning(logDBus) << "Vector index deletion failed:" << reply.errorMessage();
    }
    return result;
}

QStringList EmbeddingServer::searchVecor(const QString &query, int topK, AssistantType type)
{
    QString result = embeddingSearch(query, topK, type);
    QStringList contents;
    QJsonObject resultObj = QJsonDocument::fromJson(result.toUtf8()).object();
    for (auto res : resultObj["result"].toArray()) {
        contents += res.toObject()["content"].toString();
    }
    qCDebug(logDBus) << "Search completed, found" << contents.size() << "results";
    return contents;
}

QString EmbeddingServer::embeddingSearch(const QString &query, int topK, AssistantType type)
{
    QDBusMessage reply;
    embedInterface->setTimeout(60000);
    if (AssistantType::UOS_SYSTEM_ASSISTANT == type || AssistantType::DEEPIN_SYSTEM_ASSISTANT == type) {
        reply = embedInterface->call("Search", QVariant(kSystemAssistantEmbedding), QVariant(query), QVariant(topK));
    } else {
        reply = embedInterface->call("Search", QVariant(appid), QVariant(query), QVariant(topK));
    }

    if (reply.type() == QDBusMessage::MessageType::ErrorMessage) {
        return QString();
    }

    return reply.arguments().at(0).toString();
}

QVector<QPair<int, QString>> EmbeddingServer::getDocFiles()
{
    qCDebug(logDBus) << "Getting document files";
    
    QVector<QPair<int, QString>> docs;

    QJsonObject resultObj = getDocList();
    int version = resultObj["version"].toInt();

    if (version == 1) {
        for (auto res : resultObj["result"].toArray()) {
            docs.push_back(qMakePair(-1, res.toObject()["doc"].toString()));
        }
    } else if (version == 2) {
        for (auto res : resultObj["result"].toArray()) {
            docs.push_back(qMakePair(res.toObject()["status"].toInt(), res.toObject()["doc"].toString()));
        }
    } else {
        qCWarning(logDBus) << "Invalid document list version:" << version;
        return {};
    }

    qCDebug(logDBus) << "Retrieved" << docs.size() << "document files";
    return docs;
}

QVector<QVector<float>> EmbeddingServer::embeddingTexts(const QStringList &texts)
{
    QDBusPendingCall pendingCall = embedInterface->asyncCall("embeddingTexts", QVariant(appid), QVariant(texts));
    pendingCall.waitForFinished();
    QDBusMessage reply = pendingCall.reply();
    
    if (pendingCall.isValid()) {
        qCDebug(logDBus) << "Text embedding completed successfully";
    } else {
        qCWarning(logDBus) << "Text embedding failed:" << reply.errorMessage();
    }

    QString result = reply.arguments().at(0).toString();
    QJsonObject resultObj = QJsonDocument::fromJson(result.toUtf8()).object();

    QJsonArray embeddingsArray;
    QVector<QVector<float>> vectors;

    if (resultObj.contains("data"))
        embeddingsArray = resultObj["data"].toArray();

    for(auto embeddingObject : embeddingsArray) {
        QVector<float> tmpVectors;
        QJsonArray vectorArray = embeddingObject.toObject()["embedding"].toArray();
        for (auto value : vectorArray) {
            tmpVectors << static_cast<float>(value.toDouble());
        }
        vectors << tmpVectors;
    }

    return vectors;
}

QStringList EmbeddingServer::getDocContent()
{
    QJsonObject resultObj = getDocList();

    int version = resultObj["version"].toInt();
    
    if (version != 1 && version != 2) {
        qCWarning(logDBus) << "Invalid document list version:" << version;
        return {};
    }

    QStringList docContents;
    QJsonArray resultArray = resultObj["result"].toArray();
    int arraySize = resultArray.size();
    int startIndex = qMax(0, arraySize - 5);
    while (startIndex < arraySize) {
        QJsonObject obj = resultArray.at(startIndex).toObject();
        startIndex++;

        if (!obj.contains("content"))
            continue;

        docContents.append(obj["content"].toString());
    }

    qCDebug(logDBus) << "Retrieved" << docContents.size() << "document contents";
    return docContents;
}

void EmbeddingServer::saveAllIndex()
{
    qCDebug(logDBus) << "Saving all indexes";
    QDBusPendingCall pendingCall = embedInterface->asyncCall("saveAllIndex", QVariant(appid));
    pendingCall.waitForFinished();
}

EmbeddingServer &EmbeddingServer::getInstance()
{
    static EmbeddingServer instance;
    return instance;
}

void EmbeddingServer::onEmbedStatusChanged(const QString &app, const QStringList &files, int status)
{
    if (app == appid) {
        qCDebug(logDBus) << "Embed status changed for files:" << files << "status:" << status;
        emit addToServerStatusChanged(files, status);
    }
}

void EmbeddingServer::onIndexDeleteFinished(const QString &app, const QStringList &files)
{
    if (app == appid) {
        qCDebug(logDBus) << "Index deletion finished for files:" << files;
        emit indexDeleted(files);
    }
}
