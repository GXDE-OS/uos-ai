#include "embeddingserver.h"

#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusConnection>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

static constexpr char kSystemAssistantEmbedding[] { "SystemAssistant" };
static constexpr char kPersonlAssistantEmbedding[] { "uos-ai" };

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

    appid = QCoreApplication::applicationName();
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
    qInfo() << "update";
    if (!embedInterface->isValid()) {
        qWarning() << "Failed to create remote interface!";
        return false;
    }
    QDBusPendingCall pendingCall = embedInterface->asyncCall("Create", QVariant(appid), QVariant(files));
    pendingCall.waitForFinished();
    QDBusMessage reply = pendingCall.reply();
    bool result = reply.arguments().at(0).toBool();
    if (pendingCall.isValid()) {
        qDebug() << "Method call successful.";
    } else {
        qDebug() << "Method call failed. Error:" << reply.errorMessage();
    }
    return result;
}

bool EmbeddingServer::deleteVectorIndex(const QStringList &files)
{
    qInfo() << "delete";
    if (!embedInterface->isValid()) {
        qWarning() << "Failed to create remote interface!";
        return false;
    }
    QDBusPendingCall pendingCall = embedInterface->asyncCall("Delete", QVariant(appid), QVariant(files));
    pendingCall.waitForFinished();
    QDBusMessage reply = pendingCall.reply();
    bool result = reply.arguments().at(0).toBool();
    if (pendingCall.isValid()) {
        qDebug() << "Method call successful.";
    } else {
        qDebug() << "Method call failed. Error:" << reply.errorMessage();
    }
    return result;
}

QStringList EmbeddingServer::searchVecor(const QString &query, int topK, AssistantType type)
{
    if (!embedInterface->isValid()) {
        qWarning() << "Failed to create remote interface!";
        return {};
    }
    QDBusMessage reply;
    bool callOk = false;
    if (AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT == type) {
        QDBusPendingCall pendingCall = embedInterface->asyncCall("Search", QVariant(appid), QVariant(query), QVariant(topK));
        pendingCall.waitForFinished();
        reply = pendingCall.reply();
        callOk = pendingCall.isValid();
    } else if (AssistantType::UOS_SYSTEM_ASSISTANT == type || AssistantType::DEEPIN_SYSTEM_ASSISTANT == type) {
        QDBusPendingCall pendingCall = embedInterface->asyncCall("Search", QVariant(kSystemAssistantEmbedding), QVariant(query), QVariant(topK));
        pendingCall.waitForFinished();
        reply = pendingCall.reply();
        callOk = pendingCall.isValid();
    }

    if (callOk) {
        qDebug() << "Method call successful.";
    } else {
        qDebug() << "Method call failed. Error:" << reply.errorMessage();
    }

    //json格式检索结果
    QString result = reply.arguments().at(0).toString();
    QStringList contents;
    QJsonObject resultObj = QJsonDocument::fromJson(result.toUtf8()).object();
    for (auto res : resultObj["result"].toArray()) {
        contents += res.toObject()["content"].toString();
    }
    return contents;
}

QStringList EmbeddingServer::getDocFiles()
{
    if (!embedInterface->isValid()) {
        qWarning() << "Failed to create remote interface!";
        return {};
    }
    QDBusPendingCall pendingCall = embedInterface->asyncCall("DocFiles", QVariant(appid));
    pendingCall.waitForFinished();
    QDBusMessage reply = pendingCall.reply();
    if (pendingCall.isValid()) {
        qDebug() << "Method call successful";
    } else {
        qDebug() << "Method call failed. Error:" << reply.errorMessage();
    }

    QString result = reply.arguments().at(0).toString();

    QStringList docs;
    QJsonObject resultObj = QJsonDocument::fromJson(result.toUtf8()).object();
    if (resultObj["version"].toInt() != 1)
        return {};

    for (auto res : resultObj["result"].toArray()) {
        docs += res.toObject()["doc"].toString();
    }

    return docs;
}

QStringList EmbeddingServer::getDocContent()
{
    QStringList docContents;
    if (!embedInterface->isValid()) {
        qWarning() << "Failed to create remote interface!";
        return {};
    }
    QDBusPendingCall pendingCall = embedInterface->asyncCall("DocFiles", QVariant(appid));
    pendingCall.waitForFinished();
    QDBusMessage reply = pendingCall.reply();
    if (pendingCall.isValid()) {
        qDebug() << "Method call successful";
    } else {
        qDebug() << "Method call failed. Error:" << reply.errorMessage();
    }

    QString result = reply.arguments().at(0).toString();
    QJsonObject resultObj = QJsonDocument::fromJson(result.toUtf8()).object();
    if (resultObj["version"].toInt() != 1)
        return {};

    QJsonArray resultArray = resultObj["result"].toArray();
    int arraySize = resultArray.size();
    int startIndex = qMax(0, arraySize - 5);
    for (int i = startIndex; i < arraySize; ++i) {
        QJsonObject obj = resultArray.at(i).toObject();
        docContents.append(obj["content"].toString());
    }

    return docContents;
}

void EmbeddingServer::saveAllIndex()
{
    if (!embedInterface->isValid()) {
        qWarning() << "Failed to create remote interface!";
        return;
    }
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
    if (app == appid)
        emit addToServerStatusChanged(files, status);
}

void EmbeddingServer::onIndexDeleteFinished(const QString &app, const QStringList &files)
{
    if (app == appid)
        emit indexDeleted(files);
}
