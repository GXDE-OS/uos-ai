#include "telegramchannel.h"

#include "network/networkproxyhelper.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QUuid>

Q_LOGGING_CATEGORY(logTelegram, "uos-ai.chatbot.telegram")

using namespace uos_ai::chatbot;

static QString telegramIdToString(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();

    const QVariant variant = value.toVariant();
    if (variant.canConvert<qlonglong>())
        return QString::number(variant.toLongLong());

    const double numeric = value.toDouble();
    return numeric == 0.0 ? QString() : QString::number(static_cast<qlonglong>(numeric));
}

static QString telegramUserName(const QJsonObject &from)
{
    const QString username = from.value("username").toString().trimmed();
    if (!username.isEmpty())
        return username;

    const QString firstName = from.value("first_name").toString().trimmed();
    const QString lastName = from.value("last_name").toString().trimmed();
    const QString fullName = QStringLiteral("%1 %2").arg(firstName, lastName).trimmed();
    return fullName.isEmpty() ? telegramIdToString(from.value("id")) : fullName;
}

TelegramChannel::TelegramChannel(QObject *parent)
    : AbstractChannel(parent)
    , m_http(new QNetworkAccessManager(this))
    , m_pollTimer(new QTimer(this))
{
    m_pollTimer->setSingleShot(true);
    connect(m_pollTimer, &QTimer::timeout, this, &TelegramChannel::pollUpdates);

    if (auto *watcher = uos_ai::ProxySettingsWatcher::instance()) {
        connect(watcher, &uos_ai::ProxySettingsWatcher::proxySettingsChanged,
                this, [this] {
            applyCurrentProxy();
        });
        m_proxyWatcherConnected = true;
    }
}

void TelegramChannel::start(const QJsonObject &config)
{
    m_botToken = config.value("bot_token").toString().trimmed();
    m_apiBase = config.value("api_base").toString().trimmed();
    if (m_apiBase.isEmpty())
        m_apiBase = QStringLiteral("https://api.telegram.org");
    m_apiBase.remove(QRegularExpression(QStringLiteral("/+$")));

    if (m_botToken.isEmpty()) {
        const QString error = QObject::tr("Missing Telegram bot token.");
        qCWarning(logTelegram) << error;
        emit errorOccurred(error);
        return;
    }

    applyCurrentProxy();

    m_running = true;
    m_pollInFlight = false;
    m_offset = 0;
    m_processedUpdateIds.clear();
    m_processedUpdateOrder.clear();
    m_streamingReplies.clear();

    initializeOffset();
}

void TelegramChannel::stop()
{
    m_running = false;
    m_pollInFlight = false;
    m_pollTimer->stop();
    m_streamingReplies.clear();
}

void TelegramChannel::sendMessage(const QString &to, const QString &content,
                                  const QString &conversationType)
{
    Q_UNUSED(conversationType);

    QJsonObject body;
    body["chat_id"] = to;
    body["text"] = content;

    postJson(QStringLiteral("sendMessage"), body,
             [](const QJsonValue &) {},
             [this, to](const QString &error) {
        qCWarning(logTelegram) << "sendMessage failed for" << to << ":" << error;
        emit errorOccurred(error);
    });
}

QString TelegramChannel::beginStreamingReply(const QString &to,
                                             const QString &conversationType)
{
    const QString handle = QUuid::createUuid().toString(QUuid::WithoutBraces);

    StreamingReply reply;
    reply.to = to;
    reply.conversationType = conversationType;
    m_streamingReplies.insert(handle, reply);

    QJsonObject body;
    body["chat_id"] = to;
    body["text"] = QObject::tr("Generating...");

    postJson(QStringLiteral("sendMessage"), body,
             [this, handle](const QJsonValue &resultValue) {
        const QJsonObject result = resultValue.toObject();
        auto it = m_streamingReplies.find(handle);
        if (it == m_streamingReplies.end())
            return;

        it->ready = true;
        it->messageId = telegramIdToString(result.value("message_id"));
        if (it->messageId.isEmpty()) {
            failStreamingReply(handle, QObject::tr("Telegram placeholder message missing message_id."));
            return;
        }

        flushStreamingReply(handle);
    },
             [this, handle](const QString &error) {
        failStreamingReply(handle, error);
    });

    return handle;
}

void TelegramChannel::updateStreamingReply(const QString &streamHandle,
                                           const QString &content)
{
    auto it = m_streamingReplies.find(streamHandle);
    if (it == m_streamingReplies.end())
        return;

    it->pendingContent = content;
    flushStreamingReply(streamHandle);
}

void TelegramChannel::finalizeStreamingReply(const QString &streamHandle)
{
    auto it = m_streamingReplies.find(streamHandle);
    if (it == m_streamingReplies.end())
        return;

    it->pendingFinalize = true;
    if (it->failed) {
        if (!it->pendingContent.isEmpty())
            sendMessage(it->to, it->pendingContent, it->conversationType);
        m_streamingReplies.erase(it);
        return;
    }

    flushStreamingReply(streamHandle);
}

void TelegramChannel::initializeOffset()
{
    QJsonObject body;
    body["timeout"] = 0;
    body["limit"] = 100;

    postJson(QStringLiteral("getUpdates"), body,
             [this](const QJsonValue &resultValue) {
        if (!m_running)
            return;

        qint64 maxUpdateId = 0;
        const QJsonArray updates = resultValue.toArray();
        for (const QJsonValue &value : updates) {
            const qint64 updateId = static_cast<qint64>(value.toObject().value("update_id").toDouble(0));
            if (updateId > maxUpdateId)
                maxUpdateId = updateId;
        }
        if (maxUpdateId > 0)
            m_offset = maxUpdateId + 1;

        scheduleNextPoll();
    },
             [this](const QString &error) {
        if (!m_running)
            return;

        qCWarning(logTelegram) << "Failed to initialize Telegram offset:" << error;
        scheduleNextPoll(kRetryDelayMs);
    });
}

void TelegramChannel::pollUpdates()
{
    if (!m_running || m_pollInFlight)
        return;

    m_pollInFlight = true;

    QJsonObject body;
    body["timeout"] = kPollTimeoutSecs;
    body["offset"] = QString::number(m_offset);
    body["allowed_updates"] = QJsonArray{QStringLiteral("message")};

    postJson(QStringLiteral("getUpdates"), body,
             [this](const QJsonValue &resultValue) {
        m_pollInFlight = false;
        if (!m_running)
            return;

        handleUpdates(resultValue.toArray());
        scheduleNextPoll();
    },
             [this](const QString &error) {
        m_pollInFlight = false;
        if (!m_running)
            return;

        qCWarning(logTelegram) << "Telegram polling failed:" << error;
        scheduleNextPoll(kRetryDelayMs);
    });
}

void TelegramChannel::scheduleNextPoll(int delayMs)
{
    if (!m_running)
        return;

    m_pollTimer->start(delayMs);
}

void TelegramChannel::handleUpdates(const QJsonArray &updates)
{
    for (const QJsonValue &value : updates)
        handleUpdate(value.toObject());
}

void TelegramChannel::handleUpdate(const QJsonObject &update)
{
    const qint64 updateId = static_cast<qint64>(update.value("update_id").toDouble(0));
    if (updateId > 0) {
        if (hasProcessedUpdateId(updateId))
            return;
        appendProcessedUpdateId(updateId);
        if (updateId >= m_offset)
            m_offset = updateId + 1;
    }

    const QJsonObject message = update.value("message").toObject();
    if (message.isEmpty())
        return;

    const QString text = message.value("text").toString().trimmed();
    if (text.isEmpty())
        return;

    const QJsonObject from = message.value("from").toObject();
    if (from.value("is_bot").toBool(false))
        return;

    const QJsonObject chat = message.value("chat").toObject();
    const QString chatType = chat.value("type").toString();
    QString convType;
    if (chatType == QLatin1String("private")) {
        convType = QStringLiteral("user");
    } else if (chatType == QLatin1String("group") || chatType == QLatin1String("supergroup")) {
        convType = QStringLiteral("group");
    } else {
        return;
    }

    const QString senderId = telegramIdToString(from.value("id"));
    const QString chatId = telegramIdToString(chat.value("id"));
    if (senderId.isEmpty() || chatId.isEmpty())
        return;

    QJsonObject payload;
    payload["platform"] = platformName();
    payload["message_id"] = telegramIdToString(message.value("message_id"));
    payload["sender"] = QJsonObject{{"id", senderId}, {"name", telegramUserName(from)}, {"type", "user"}};
    payload["conversation"] = QJsonObject{{"id", chatId}, {"type", convType}};
    payload["content"] = QJsonObject{{"type", "text"}, {"text", text}};
    payload["timestamp"] = static_cast<qint64>(message.value("date").toDouble(QDateTime::currentSecsSinceEpoch()));

    emit messageReceived(payload);
}

void TelegramChannel::flushStreamingReply(const QString &handle)
{
    auto it = m_streamingReplies.find(handle);
    if (it == m_streamingReplies.end())
        return;

    if (it->failed) {
        if (it->pendingFinalize && !it->pendingContent.isEmpty()) {
            sendMessage(it->to, it->pendingContent, it->conversationType);
            m_streamingReplies.erase(it);
        }
        return;
    }

    if (!it->ready || it->requestInFlight)
        return;

    const QString nextContent = it->pendingContent;
    if (nextContent.isEmpty() || nextContent == it->lastSentContent) {
        if (it->pendingFinalize && !it->requestInFlight)
            m_streamingReplies.erase(it);
        return;
    }

    it->requestInFlight = true;

    QJsonObject body;
    body["chat_id"] = it->to;
    body["message_id"] = it->messageId;
    body["text"] = nextContent;

    postJson(QStringLiteral("editMessageText"), body,
             [this, handle, nextContent](const QJsonValue &) {
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
    },
             [this, handle](const QString &error) {
        failStreamingReply(handle, error);
    });
}

void TelegramChannel::failStreamingReply(const QString &handle, const QString &error)
{
    auto it = m_streamingReplies.find(handle);
    if (it == m_streamingReplies.end())
        return;

    qCWarning(logTelegram) << "Telegram streaming failed:" << error;
    const QString finalContent = it->pendingContent;
    const QString to = it->to;
    const QString conversationType = it->conversationType;
    const bool shouldSendFallback = it->pendingFinalize && !finalContent.isEmpty();
    m_streamingReplies.erase(it);

    if (shouldSendFallback)
        sendMessage(to, finalContent, conversationType);
}

QString TelegramChannel::apiUrl(const QString &method) const
{
    return QStringLiteral("%1/bot%2/%3").arg(m_apiBase, m_botToken, method);
}

void TelegramChannel::postJson(const QString &method, const QJsonObject &body,
                               std::function<void(const QJsonValue &result)> onSuccess,
                               std::function<void(const QString &error)> onError)
{
    QNetworkRequest request((QUrl(apiUrl(method))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_http->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, onSuccess = std::move(onSuccess), onError = std::move(onError)] {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            onError(reply->errorString());
            return;
        }

        const QByteArray raw = reply->readAll();
        const QJsonObject obj = QJsonDocument::fromJson(raw).object();
        if (!obj.value("ok").toBool(false)) {
            onError(obj.value("description").toString(QObject::tr("Telegram API request failed.")));
            return;
        }

        onSuccess(obj.value("result"));
    });
}

void TelegramChannel::appendProcessedUpdateId(qint64 updateId)
{
    if (updateId <= 0 || m_processedUpdateIds.contains(updateId))
        return;

    m_processedUpdateIds.insert(updateId);
    m_processedUpdateOrder.append(updateId);
    while (m_processedUpdateOrder.size() > kMaxProcessedIds) {
        const qint64 oldest = m_processedUpdateOrder.takeFirst();
        m_processedUpdateIds.remove(oldest);
    }
}

bool TelegramChannel::hasProcessedUpdateId(qint64 updateId) const
{
    return m_processedUpdateIds.contains(updateId);
}

void TelegramChannel::applyCurrentProxy()
{
    if (m_apiBase.isEmpty())
        return;

    ProxySettingsWrapper::applyProxyToNetworkAccessManager(m_http, QUrl(m_apiBase), logTelegram(), "Telegram");
}
