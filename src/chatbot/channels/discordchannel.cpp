#include "discordchannel.h"

#include "network/networkproxyhelper.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslError>
#include <QUrl>
#include <QUuid>

Q_LOGGING_CATEGORY(logDiscord, "uos-ai.chatbot.discord")

using namespace uos_ai::chatbot;

static QString discordIdToString(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();

    const QVariant variant = value.toVariant();
    if (variant.canConvert<qlonglong>())
        return QString::number(variant.toLongLong());

    const double numeric = value.toDouble();
    return numeric == 0.0 ? QString() : QString::number(static_cast<qlonglong>(numeric));
}

static QString redactSecret(const QString &value)
{
    if (value.size() <= 8)
        return QStringLiteral("***");
    return value.left(4) + QStringLiteral("...") + value.right(4);
}

static QJsonValue sanitizedGatewayData(const QJsonValue &data)
{
    if (!data.isObject())
        return data;

    QJsonObject object = data.toObject();
    if (object.contains(QStringLiteral("token")))
        object[QStringLiteral("token")] = redactSecret(object.value(QStringLiteral("token")).toString());
    if (object.contains(QStringLiteral("session_id")))
        object[QStringLiteral("session_id")] = redactSecret(object.value(QStringLiteral("session_id")).toString());
    return object;
}

DiscordChannel::DiscordChannel(QObject *parent)
    : AbstractChannel(parent)
    , m_ws(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_http(new QNetworkAccessManager(this))
    , m_heartbeatTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
{
    m_heartbeatTimer->setSingleShot(false);
    m_reconnectTimer->setSingleShot(true);

    connect(m_ws, &QWebSocket::connected, this, [this] {
        qCInfo(logDiscord) << "Discord gateway connected";
    });
    connect(m_ws, &QWebSocket::disconnected, this, [this] {
        m_heartbeatTimer->stop();
        qCInfo(logDiscord) << "Discord gateway disconnected";
        if (m_running)
            scheduleReconnect();
    });
    connect(m_ws, &QWebSocket::textMessageReceived,
            this, &DiscordChannel::handleGatewayMessage);
    connect(m_ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
            [this](QAbstractSocket::SocketError) {
        qCWarning(logDiscord) << "Discord gateway error:" << m_ws->error()
                             << m_ws->errorString();
        emit errorOccurred(QObject::tr("Discord gateway error: %1").arg(m_ws->errorString()));
    });
    connect(m_ws, &QWebSocket::sslErrors, this, [this](const QList<QSslError> &errors) {
        for (const QSslError &error : errors)
            qCWarning(logDiscord) << "Discord gateway SSL error:" << error.errorString();
    });

    connect(m_heartbeatTimer, &QTimer::timeout, this, &DiscordChannel::sendHeartbeat);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this] { fetchGateway(); });

    if (auto *watcher = uos_ai::ProxySettingsWatcher::instance()) {
        connect(watcher, &uos_ai::ProxySettingsWatcher::proxySettingsChanged,
                this, &DiscordChannel::handleProxySettingsChanged);
        m_proxyWatcherConnected = true;
    }
}

void DiscordChannel::start(const QJsonObject &config)
{
    m_botToken = config.value("bot_token").toString().trimmed();
    m_applicationId = config.value("application_id").toString().trimmed();
    m_guildId = config.value("guild_id").toString().trimmed();

    if (m_botToken.isEmpty()) {
        const QString error = QObject::tr("Missing Discord bot token.");
        qCWarning(logDiscord) << error;
        emit errorOccurred(error);
        return;
    }

    applyCurrentProxy();

    m_running = true;
    m_lastSequence = 0;
    m_sessionId.clear();
    m_resumeGatewayUrl.clear();
    m_commandsSynced = false;
    m_commandsSyncInFlight = false;
    m_dmChannels.clear();
    m_pendingInteractionContexts.clear();
    m_streamingReplies.clear();
    fetchGateway();
}

void DiscordChannel::stop()
{
    m_running = false;
    m_gatewayFetchInFlight = false;
    m_heartbeatTimer->stop();
    m_reconnectTimer->stop();
    m_pendingInteractionContexts.clear();
    m_streamingReplies.clear();

    if (m_ws->state() != QAbstractSocket::UnconnectedState)
        m_ws->close();
}

void DiscordChannel::sendMessage(const QString &to, const QString &content,
                                 const QString &conversationType)
{
    resolveChannelId(to, conversationType,
                     [this, content](const QString &channelId) {
        sendChannelMessage(channelId, content,
                           [](const QJsonObject &) {},
                           [this](const QString &error) { emit errorOccurred(error); });
    },
                     [this](const QString &error) { emit errorOccurred(error); });
}

QString DiscordChannel::beginStreamingReply(const QString &to,
                                            const QString &conversationType)
{
    const QString handle = QUuid::createUuid().toString(QUuid::WithoutBraces);

    StreamingReply reply;
    reply.targetId = to;
    reply.conversationType = conversationType;

    auto interactionIt = m_pendingInteractionContexts.find(to);
    if (interactionIt != m_pendingInteractionContexts.end() && !interactionIt.value().isEmpty()) {
        const InteractionContext context = interactionIt.value().takeFirst();
        if (interactionIt.value().isEmpty())
            m_pendingInteractionContexts.erase(interactionIt);
        reply.interactionMode = true;
        reply.channelId = context.channelId;
        reply.interactionId = context.interactionId;
        reply.interactionToken = context.interactionToken;
    }
    m_streamingReplies.insert(handle, reply);

    if (reply.interactionMode) {
        deferInteractionResponse(reply.interactionId, reply.interactionToken,
                                 [this, handle]() {
            auto it = m_streamingReplies.find(handle);
            if (it == m_streamingReplies.end())
                return;
            it->ready = true;
            flushStreamingReply(handle);
        },
                                 [this, handle](const QString &error) {
            failStreamingReply(handle, error);
        });
        return handle;
    }

    resolveChannelId(to, conversationType,
                     [this, handle](const QString &channelId) {
        auto it = m_streamingReplies.find(handle);
        if (it == m_streamingReplies.end())
            return;

        it->channelId = channelId;
        sendChannelMessage(channelId, QObject::tr("Generating..."),
                           [this, handle](const QJsonObject &result) {
            auto current = m_streamingReplies.find(handle);
            if (current == m_streamingReplies.end())
                return;

            current->ready = true;
            current->messageId = discordIdToString(result.value("id"));
            if (current->messageId.isEmpty()) {
                failStreamingReply(handle, QObject::tr("Discord placeholder message missing id."));
                return;
            }

            flushStreamingReply(handle);
        },
                           [this, handle](const QString &error) {
            failStreamingReply(handle, error);
        });
    },
                     [this, handle](const QString &error) {
        failStreamingReply(handle, error);
    });

    return handle;
}

void DiscordChannel::updateStreamingReply(const QString &streamHandle,
                                          const QString &content)
{
    auto it = m_streamingReplies.find(streamHandle);
    if (it == m_streamingReplies.end())
        return;

    it->pendingContent = content;
    flushStreamingReply(streamHandle);
}

void DiscordChannel::finalizeStreamingReply(const QString &streamHandle)
{
    auto it = m_streamingReplies.find(streamHandle);
    if (it == m_streamingReplies.end())
        return;

    it->pendingFinalize = true;
    if (it->failed) {
        if (!it->pendingContent.isEmpty())
            sendMessage(it->targetId, it->pendingContent, it->conversationType);
        m_streamingReplies.erase(it);
        return;
    }

    flushStreamingReply(streamHandle);
}

void DiscordChannel::fetchGateway()
{
    if (!m_running || m_gatewayFetchInFlight)
        return;

    m_gatewayFetchInFlight = true;
    requestJson(QStringLiteral("GET"), QStringLiteral("/gateway/bot"), QJsonObject(),
                [this](const QJsonObject &result) {
        m_gatewayFetchInFlight = false;
        if (!m_running)
            return;

        m_gatewayUrl = result.value("url").toString();
        if (m_gatewayUrl.isEmpty()) {
            emit errorOccurred(QObject::tr("Discord gateway URL is empty."));
            scheduleReconnect();
            return;
        }
        if (!m_commandsSynced && !m_commandsSyncInFlight)
            syncCommands();
        connectGateway();
    },
                [this](const QString &error) {
        m_gatewayFetchInFlight = false;
        emit errorOccurred(error);
        scheduleReconnect();
    });
}

void DiscordChannel::connectGateway()
{
    if (!m_running)
        return;

    QString url = !m_resumeGatewayUrl.isEmpty() ? m_resumeGatewayUrl : m_gatewayUrl;
    if (url.isEmpty())
        return;

    if (!url.contains(QStringLiteral("?")))
        url += QStringLiteral("?v=10&encoding=json");
    else if (!url.contains(QStringLiteral("encoding=json")))
        url += QStringLiteral("&v=10&encoding=json");

    qCInfo(logDiscord) << "Connecting Discord gateway:" << url;
    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("User-Agent", QByteArrayLiteral("uos-ai-assistant/1.0"));
    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::AutoVerifyPeer);
    request.setSslConfiguration(sslConfig);
    m_ws->open(request);
}

void DiscordChannel::scheduleReconnect(int delayMs)
{
    if (!m_running)
        return;

    qCInfo(logDiscord) << "Scheduling Discord reconnect in" << delayMs << "ms";
    m_reconnectTimer->start(delayMs);
}

void DiscordChannel::sendGatewayPayload(int op, const QJsonValue &data)
{
    QJsonObject payload;
    payload["op"] = op;
    payload["d"] = data;
    qCInfo(logDiscord) << "Sending Discord gateway payload op=" << op
                       << "data=" << QJsonDocument::fromVariant(sanitizedGatewayData(data).toVariant()).toJson(QJsonDocument::Compact);
    m_ws->sendTextMessage(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact)));
}

void DiscordChannel::sendHeartbeat()
{
    if (!m_running)
        return;

    if (!m_heartbeatAcked) {
        qCWarning(logDiscord) << "Discord heartbeat ack timeout, reconnecting";
        m_ws->close();
        return;
    }

    m_heartbeatAcked = false;
    sendGatewayPayload(1, m_lastSequence == 0 ? QJsonValue() : QJsonValue(QString::number(m_lastSequence)));
}

void DiscordChannel::handleGatewayMessage(const QString &message)
{
    const QJsonObject obj = QJsonDocument::fromJson(message.toUtf8()).object();
    const int op = obj.value("op").toInt(-1);
    const QString eventType = obj.value("t").toString();
    const QJsonValue seqValue = obj.value("s");
    if (!seqValue.isNull())
        m_lastSequence = static_cast<qint64>(seqValue.toDouble(m_lastSequence));

    switch (op) {
    case 10: {
        const QJsonObject hello = obj.value("d").toObject();
        m_heartbeatIntervalMs = hello.value("heartbeat_interval").toInt(45000);
        m_heartbeatAcked = true;
        m_heartbeatTimer->start(m_heartbeatIntervalMs);

        if (!m_sessionId.isEmpty()) {
            QJsonObject resume;
            resume["token"] = m_botToken;
            resume["session_id"] = m_sessionId;
            resume["seq"] = QString::number(m_lastSequence);
            sendGatewayPayload(6, resume);
        } else {
            const int identifyIntents = kIntentGuilds | kIntentGuildMessages | kIntentDirectMessages;
            QJsonObject identify;
            identify["token"] = m_botToken;
            identify["intents"] = identifyIntents;
            identify["properties"] = QJsonObject{
                {"$os", QStringLiteral("linux")},
                {"$browser", QStringLiteral("uos-ai-assistant")},
                {"$device", QStringLiteral("uos-ai-assistant")}
            };
            sendGatewayPayload(2, identify);
        }
        break;
    }
    case 11:
        m_heartbeatAcked = true;
        break;
    case 7:
        m_ws->close();
        break;
    case 9:
        m_sessionId.clear();
        m_resumeGatewayUrl.clear();
        m_lastSequence = 0;
        m_ws->close();
        break;
    case 1:
        sendHeartbeat();
        break;
    case 0:
        handleDispatch(obj.value("t").toString(), obj.value("d").toObject());
        break;
    default:
        break;
    }
}

void DiscordChannel::handleDispatch(const QString &eventType, const QJsonObject &data)
{
    if (eventType == QLatin1String("READY")) {
        m_sessionId = data.value("session_id").toString();
        m_resumeGatewayUrl = data.value("resume_gateway_url").toString();
        return;
    }

    if (eventType == QLatin1String("RESUMED"))
        return;

    if (eventType == QLatin1String("MESSAGE_CREATE"))
        handleMessageCreate(data);
    else if (eventType == QLatin1String("INTERACTION_CREATE"))
        handleInteractionCreate(data);
}

void DiscordChannel::handleMessageCreate(const QJsonObject &data)
{
    const QJsonObject author = data.value("author").toObject();
    if (author.value("bot").toBool(false))
        return;

    const QString text = data.value("content").toString().trimmed();
    if (text.isEmpty())
        return;

    const QString channelId = discordIdToString(data.value("channel_id"));
    if (channelId.isEmpty())
        return;

    QString convType;
    QString convId;
    const QString guildId = discordIdToString(data.value("guild_id"));
    const QString senderId = discordIdToString(author.value("id"));
    if (senderId.isEmpty())
        return;

    if (guildId.isEmpty()) {
        convType = QStringLiteral("user");
        convId = senderId;
        m_dmChannels[senderId] = channelId;
    } else {
        convType = QStringLiteral("group");
        convId = channelId;
    }

    QJsonObject payload;
    payload["platform"] = platformName();
    payload["message_id"] = discordIdToString(data.value("id"));
    payload["sender"] = QJsonObject{{"id", senderId},
                                     {"name", author.value("username").toString(senderId)},
                                     {"type", "user"}};
    payload["conversation"] = QJsonObject{{"id", convId}, {"type", convType}};
    payload["content"] = QJsonObject{{"type", "text"}, {"text", text}};
    payload["timestamp"] = QDateTime::currentSecsSinceEpoch();

    emit messageReceived(payload);
}

void DiscordChannel::handleInteractionCreate(const QJsonObject &data)
{
    const QJsonObject interactionData = data.value("data").toObject();
    const QString commandName = interactionData.value("name").toString();
    if (commandName != QLatin1String("ai") && commandName != QLatin1String("uosai"))
        return;

    QString prompt;
    const QJsonArray options = interactionData.value("options").toArray();
    for (const QJsonValue &value : options) {
        const QJsonObject option = value.toObject();
        if (option.value("name").toString() == QLatin1String("prompt")) {
            prompt = option.value("value").toString().trimmed();
            break;
        }
    }
    if (prompt.isEmpty())
        return;

    const QJsonObject member = data.value("member").toObject();
    const QJsonObject user = member.value("user").toObject().isEmpty()
                             ? data.value("user").toObject()
                             : member.value("user").toObject();
    const QString senderId = discordIdToString(user.value("id"));
    const QString channelId = discordIdToString(data.value("channel_id"));
    if (senderId.isEmpty() || channelId.isEmpty())
        return;

    const QString guildId = discordIdToString(data.value("guild_id"));
    const QString convType = guildId.isEmpty() ? QStringLiteral("user") : QStringLiteral("group");
    const QString convId = guildId.isEmpty() ? senderId : channelId;
    const QString interactionId = discordIdToString(data.value("id"));
    const QString interactionToken = data.value("token").toString();
    if (interactionId.isEmpty() || interactionToken.isEmpty())
        return;

    m_pendingInteractionContexts[convId].append({ interactionId, interactionToken, channelId });
    if (guildId.isEmpty())
        m_dmChannels[senderId] = channelId;

    QJsonObject payload;
    payload["platform"] = platformName();
    payload["message_id"] = interactionId;
    payload["sender"] = QJsonObject{{"id", senderId},
                                     {"name", user.value("username").toString(senderId)},
                                     {"type", "user"}};
    payload["conversation"] = QJsonObject{{"id", convId}, {"type", convType}};
    payload["content"] = QJsonObject{{"type", "text"},
                                      {"text", prompt},
                                      {"source", "slash_command"}};
    payload["meta"] = QJsonObject{{"interaction_id", interactionId},
                                   {"interaction_token", interactionToken},
                                   {"command_name", commandName},
                                   {"response_mode", "interaction"}};
    payload["timestamp"] = QDateTime::currentSecsSinceEpoch();

    emit messageReceived(payload);
}

void DiscordChannel::syncCommands()
{
    if (m_applicationId.isEmpty() || m_commandsSynced || m_commandsSyncInFlight)
        return;

    m_commandsSyncInFlight = true;

    const QString basePath = m_guildId.isEmpty()
                             ? QStringLiteral("/applications/%1/commands").arg(m_applicationId)
                             : QStringLiteral("/applications/%1/guilds/%2/commands").arg(m_applicationId, m_guildId);

    const QJsonArray commands = {
        buildCommandDefinition(QStringLiteral("ai")),
        buildCommandDefinition(QStringLiteral("uosai"))
    };

    requestJsonValue(QStringLiteral("PUT"), basePath, commands,
                     [this](const QJsonValue &) {
        m_commandsSyncInFlight = false;
        m_commandsSynced = true;
    },
                     [this](const QString &error) {
        m_commandsSyncInFlight = false;
        qCWarning(logDiscord) << "Failed to sync Discord commands:" << error;
    });
}

QJsonObject DiscordChannel::buildCommandDefinition(const QString &commandName) const
{
    return QJsonObject{
        {"name", commandName},
        {"description", QObject::tr("Ask UOS AI a question")},
        {"type", 1},
        {"options", QJsonArray{
            QJsonObject{{"type", 3},
                        {"name", QStringLiteral("prompt")},
                        {"description", QObject::tr("Question for UOS AI")},
                        {"required", true}}
        }}
    };
}

void DiscordChannel::resolveChannelId(const QString &to, const QString &conversationType,
                                      std::function<void(const QString &channelId)> onReady,
                                      std::function<void(const QString &error)> onError)
{
    if (conversationType != QLatin1String("user")) {
        onReady(to);
        return;
    }

    const QString cached = m_dmChannels.value(to);
    if (!cached.isEmpty()) {
        onReady(cached);
        return;
    }

    createDmChannel(to, std::move(onReady), std::move(onError));
}

void DiscordChannel::createDmChannel(const QString &userId,
                                     std::function<void(const QString &channelId)> onReady,
                                     std::function<void(const QString &error)> onError)
{
    QJsonObject body;
    body["recipient_id"] = userId;
    requestJson(QStringLiteral("POST"), QStringLiteral("/users/@me/channels"), body,
                [this, userId, onReady = std::move(onReady)](const QJsonObject &result) {
        const QString channelId = discordIdToString(result.value("id"));
        if (channelId.isEmpty()) {
            onReady(QString());
            return;
        }
        m_dmChannels[userId] = channelId;
        onReady(channelId);
    },
                std::move(onError));
}

void DiscordChannel::sendChannelMessage(const QString &channelId, const QString &content,
                                        std::function<void(const QJsonObject &result)> onSuccess,
                                        std::function<void(const QString &error)> onError)
{
    QJsonObject body;
    body["content"] = content;
    requestJson(QStringLiteral("POST"),
                QStringLiteral("/channels/%1/messages").arg(channelId),
                body, std::move(onSuccess), std::move(onError));
}

void DiscordChannel::editChannelMessage(const QString &channelId, const QString &messageId,
                                        const QString &content,
                                        std::function<void()> onSuccess,
                                        std::function<void(const QString &error)> onError)
{
    QJsonObject body;
    body["content"] = content;
    requestJson(QStringLiteral("PATCH"),
                QStringLiteral("/channels/%1/messages/%2").arg(channelId, messageId),
                body,
                [onSuccess = std::move(onSuccess)](const QJsonObject &) { onSuccess(); },
                std::move(onError));
}

void DiscordChannel::deferInteractionResponse(const QString &interactionId,
                                              const QString &interactionToken,
                                              std::function<void()> onSuccess,
                                              std::function<void(const QString &error)> onError)
{
    QJsonObject body;
    body["type"] = 5;
    requestJson(QStringLiteral("POST"),
                QStringLiteral("/interactions/%1/%2/callback").arg(interactionId, interactionToken),
                body,
                [onSuccess = std::move(onSuccess)](const QJsonObject &) { onSuccess(); },
                std::move(onError));
}

void DiscordChannel::editInteractionResponse(const QString &interactionToken, const QString &content,
                                             std::function<void()> onSuccess,
                                             std::function<void(const QString &error)> onError)
{
    QJsonObject body;
    body["content"] = content;
    requestJson(QStringLiteral("PATCH"),
                QStringLiteral("/webhooks/%1/%2/messages/@original").arg(m_applicationId, interactionToken),
                body,
                [onSuccess = std::move(onSuccess)](const QJsonObject &) { onSuccess(); },
                std::move(onError));
}

void DiscordChannel::flushStreamingReply(const QString &handle)
{
    auto it = m_streamingReplies.find(handle);
    if (it == m_streamingReplies.end())
        return;

    if (it->failed) {
        if (it->pendingFinalize && !it->pendingContent.isEmpty())
            sendMessage(it->targetId, it->pendingContent, it->conversationType);
        m_streamingReplies.erase(it);
        return;
    }

    if (!it->ready || it->requestInFlight)
        return;

    const QString nextContent = it->pendingContent;
    if (nextContent.isEmpty() || nextContent == it->lastSentContent) {
        if (it->pendingFinalize)
            m_streamingReplies.erase(it);
        return;
    }

    it->requestInFlight = true;
    const auto onSuccess = [this, handle, nextContent]() {
        auto current = m_streamingReplies.find(handle);
        if (current == m_streamingReplies.end())
            return;

        current->requestInFlight = false;
        current->lastSentContent = nextContent;
        if (current->pendingContent != current->lastSentContent) {
            flushStreamingReply(handle);
            return;
        }
        if (current->pendingFinalize)
            m_streamingReplies.erase(current);
    };
    const auto onError = [this, handle](const QString &error) {
        failStreamingReply(handle, error);
    };

    if (it->interactionMode) {
        editInteractionResponse(it->interactionToken, nextContent, onSuccess, onError);
    } else {
        editChannelMessage(it->channelId, it->messageId, nextContent, onSuccess, onError);
    }
}

void DiscordChannel::failStreamingReply(const QString &handle, const QString &error)
{
    auto it = m_streamingReplies.find(handle);
    if (it == m_streamingReplies.end())
        return;

    qCWarning(logDiscord) << "Discord streaming failed:" << error;
    const QString targetId = it->targetId;
    const QString convType = it->conversationType;
    const QString content = it->pendingContent;
    const QString interactionToken = it->interactionToken;
    const bool interactionMode = it->interactionMode;
    const bool shouldFallback = it->pendingFinalize && !content.isEmpty();
    m_streamingReplies.erase(it);

    if (shouldFallback && interactionMode && !interactionToken.isEmpty() && !m_applicationId.isEmpty()) {
        editInteractionResponse(interactionToken, content, []() {}, [](const QString &) {});
    } else if (shouldFallback) {
        sendMessage(targetId, content, convType);
    }
}

void DiscordChannel::requestJsonValue(const QString &method, const QString &path,
                                      const QJsonValue &body,
                                      std::function<void(const QJsonValue &result)> onSuccess,
                                      std::function<void(const QString &error)> onError)
{
    const QUrl requestUrl(QStringLiteral("https://discord.com/api/v10%1").arg(path));
    QNetworkRequest request(requestUrl);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
#endif
    request.setRawHeader("User-Agent", QByteArrayLiteral("uos-ai-assistant/1.0"));
    request.setRawHeader("Authorization", QStringLiteral("Bot %1").arg(m_botToken).toUtf8());

    QNetworkReply *reply = nullptr;
    const bool hasRequestBody = method != QLatin1String("GET") && !body.isUndefined();
    const QByteArray data = hasRequestBody
                            ? QJsonDocument::fromVariant(body.toVariant()).toJson(QJsonDocument::Compact)
                            : QByteArray();
    if (method != QLatin1String("GET"))
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    if (method == QLatin1String("GET"))
        reply = m_http->get(request);
    else if (method == QLatin1String("POST"))
        reply = m_http->post(request, data);
    else if (method == QLatin1String("PUT"))
        reply = m_http->put(request, data);
    else
        reply = m_http->sendCustomRequest(request, method.toUtf8(), data);

    connect(reply, &QNetworkReply::finished, this,
            [reply, onSuccess = std::move(onSuccess), onError = std::move(onError)] {
        reply->deleteLater();

        const QByteArray raw = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(raw);
        const QJsonObject resultObject = doc.isObject() ? doc.object() : QJsonObject();
        if (reply->error() != QNetworkReply::NoError) {
            const QString error = resultObject.value("message").toString(reply->errorString());
            onError(error);
            return;
        }

        if (doc.isArray())
            onSuccess(doc.array());
        else if (doc.isObject())
            onSuccess(doc.object());
        else
            onSuccess(QJsonValue());
    });
}

void DiscordChannel::requestJson(const QString &method, const QString &path,
                                 const QJsonObject &body,
                                 std::function<void(const QJsonObject &result)> onSuccess,
                                 std::function<void(const QString &error)> onError)
{
    requestJsonValue(method, path, body,
                     [onSuccess = std::move(onSuccess)](const QJsonValue &result) {
        onSuccess(result.toObject());
    },
                     std::move(onError));
}

void DiscordChannel::applyCurrentProxy()
{
    const QUrl restUrl(QStringLiteral("https://discord.com/api/v10"));
    uos_ai::applyProxyToNetworkAccessManager(m_http, restUrl, logDiscord(), "Discord HTTP");
    uos_ai::applyProxyToWebSocket(m_ws, QUrl(QStringLiteral("wss://gateway.discord.gg")),
                                  logDiscord(), "Discord WebSocket");
}

void DiscordChannel::handleProxySettingsChanged()
{
    applyCurrentProxy();

    if (!m_running)
        return;

    if (m_ws->state() == QAbstractSocket::ConnectedState
        || m_ws->state() == QAbstractSocket::ConnectingState) {
        qCInfo(logDiscord) << "Proxy settings changed, reconnecting Discord gateway";
        m_ws->close();
    }
}
