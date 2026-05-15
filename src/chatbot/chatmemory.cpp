#include "chatmemory.h"
#include "chatbotpaths.h"
#include "chatbot_key_define.h"
#include "global_key_define.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logChatBot)

using namespace uos_ai;
using namespace uos_ai::chatbot;

static QString platformFromKey(const QString &key)
{
    const int idx = key.indexOf(':');
    return (idx != -1) ? key.left(idx) : QString();
}

ChatMemory::ChatMemory(QObject *parent)
    : QObject(parent)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(SAVE_DELAY_MS);
    connect(&m_saveTimer, &QTimer::timeout, this, [this] {
        for (const QString &key : std::as_const(m_dirtyKeys))
            save(key);
        m_dirtyKeys.clear();
    });

    load();
}

// ============================================================
// 会话历史
// ============================================================

void ChatMemory::append(const QString &key, const QString &role, const QString &content,
                        const QString &senderId)
{
    const QString actualKey = resolveKey(key, senderId);
    ConvHistory &h = m_store[actualKey];
    h.lastActiveMs = QDateTime::currentMSecsSinceEpoch();
    h.messages.append({role, content, {}, {}, h.lastActiveMs});

    if (role == QLatin1String("assistant"))
        tryCompress(actualKey);

    scheduleSave(actualKey);
}

void ChatMemory::appendTurn(const QString &key, const QString &userText,
                             const QJsonArray &agentContext, const QString &senderId)
{
    const QString actualKey = resolveKey(key, senderId);
    ConvHistory &h = m_store[actualKey];
    h.lastActiveMs = QDateTime::currentMSecsSinceEpoch();

    // 存储用户消息
    h.messages.append({STR_KEY_USER, userText, {}, {}, h.lastActiveMs});

    // 存储 agent context 链（OAI 格式：assistant tool_calls + tool results + 最终 assistant）
    for (const QJsonValue &v : agentContext) {
        const QJsonObject msg = v.toObject();
        const QString role    = msg.value(STR_KEY_ROLE).toString();

        MemoryMessage m;
        m.timestamp = h.lastActiveMs;
        m.role      = role;

        if (role == QLatin1String(STR_KEY_ASSISTANT) && msg.contains(STR_KEY_TOOL_CALLS)) {
            m.toolCalls = msg.value(STR_KEY_TOOL_CALLS).toArray();
            m.content   = msg.value(STR_KEY_CONTENT).toString();
        } else if (role == QLatin1String(STR_KEY_TOOL)) {
            m.toolCallId = msg.value(STR_KEY_TOOL_CALL_ID).toString();
            m.content    = msg.value(STR_KEY_CONTENT).toString();
        } else {
            m.content = msg.value(STR_KEY_CONTENT).toString();
        }

        h.messages.append(m);
    }

    tryCompress(actualKey);
    scheduleSave(actualKey);
}

QJsonArray ChatMemory::buildContext(const QString &key, const QString &userText,
                                    const QString &senderId) const
{
    const QString actualKey = resolveKey(key, senderId);
    const ConvHistory &h    = m_store.value(actualKey);

    // 从最新消息往前取，直到字符预算耗尽（以轮次为边界）
    int charBudget = MAX_CHARS - userText.size();
    int start = h.messages.size();

    for (int i = h.messages.size() - 1; i >= 0 && charBudget > 0; --i) {
        charBudget -= messageCharCount(h.messages[i]);
        if (charBudget < 0)
            break;
        start = i;
    }

    // 对齐到轮起始（user 消息）：若 start 指向非 user 消息，跳到下一个 user
    while (start < h.messages.size()
           && h.messages[start].role != QLatin1String(STR_KEY_USER)) {
        ++start;
    }

    // 序列化为 OAI 格式
    QJsonArray raw;
    for (int i = start; i < h.messages.size(); ++i) {
        const MemoryMessage &m = h.messages[i];
        QJsonObject msg;
        msg[STR_KEY_ROLE] = m.role;

        if (!m.toolCalls.isEmpty()) {
            // assistant with tool_calls
            msg[STR_KEY_CONTENT]    = m.content.isEmpty()
                                      ? QJsonValue(QJsonValue::Null)
                                      : QJsonValue(m.content);
            msg[STR_KEY_TOOL_CALLS] = m.toolCalls;
        } else if (!m.toolCallId.isEmpty()) {
            // tool result
            msg[STR_KEY_TOOL_CALL_ID] = m.toolCallId;
            if (m.content.size() > TOOL_RESULT_MAX_CHARS) {
                msg[STR_KEY_CONTENT] = m.content.left(TOOL_RESULT_MAX_CHARS)
                    + QStringLiteral("…[truncated]");
            } else {
                msg[STR_KEY_CONTENT] = m.content;
            }
        } else {
            msg[STR_KEY_CONTENT] = m.content;
        }

        raw.append(msg);
    }

    // 校验并过滤孤立的 tool_calls / tool result
    QJsonArray arr = sanitizeToolMessages(raw);

    // 追加当前用户消息
    QJsonObject cur;
    cur[STR_KEY_ROLE]    = STR_KEY_USER;
    cur[STR_KEY_CONTENT] = userText;
    arr.append(cur);

    return arr;
}

bool ChatMemory::removeLast(const QString &key, const QString &role,
                             const QString &senderId)
{
    const QString actualKey = resolveKey(key, senderId);
    auto it = m_store.find(actualKey);
    if (it == m_store.end())
        return false;

    QList<MemoryMessage> &msgs = it->messages;
    for (int i = msgs.size() - 1; i >= 0; --i) {
        if (msgs[i].role == role) {
            msgs.removeAt(i);
            return true;
        }
    }
    return false;
}

ConvStats ChatMemory::stats(const QString &key, const QString &senderId) const
{
    const QString actualKey = resolveKey(key, senderId);
    const ConvHistory &h    = m_store.value(actualKey);

    ConvStats s;
    s.messageCount = h.messages.size();
    s.lastActiveMs = h.lastActiveMs;
    for (const MemoryMessage &m : h.messages)
        s.charCount += messageCharCount(m);
    return s;
}

void ChatMemory::clearConversation(const QString &key, const QString &senderId)
{
    const QString actualKey = resolveKey(key, senderId);
    m_store.remove(actualKey);
    m_dirtyKeys.remove(actualKey);
    const QString platform = platformFromKey(actualKey);
    if (!platform.isEmpty())
        QFile::remove(ChatBotPaths::instance().conversationFile(platform, encodeKey(actualKey)));
}

void ChatMemory::clearAll()
{
    m_saveTimer.stop();
    m_store.clear();
    m_dirtyKeys.clear();

    for (const QString &filePath : allConversationFilePaths())
        QFile::remove(filePath);

    qCDebug(logChatBot) << "All conversations cleared";
}

QStringList ChatMemory::activeKeys() const
{
    return QStringList(m_store.keys());
}

QString ChatMemory::summaryFor(const QString &key, const QString &senderId) const
{
    return m_store.value(resolveKey(key, senderId)).summary;
}

// ============================================================
// private
// ============================================================

QStringList ChatMemory::allConversationFilePaths() const
{
    QStringList result;
    const QFileInfoList platformDirs =
        QDir(ChatBotPaths::instance().dataDir())
            .entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &platformDir : platformDirs) {
        const QString dir = ChatBotPaths::instance().conversationsDir(platformDir.fileName());
        for (const QFileInfo &fi : QDir(dir).entryInfoList({ QStringLiteral("*.json") }, QDir::Files))
            result += fi.absoluteFilePath();
    }
    return result;
}

QString ChatMemory::resolveKey(const QString &key, const QString &senderId) const
{
    return senderId.isEmpty() ? key : key + ":" + senderId;
}

QString ChatMemory::encodeKey(const QString &key) const
{
    const int colonIdx = key.indexOf(':');
    QString rest = (colonIdx != -1) ? key.mid(colonIdx + 1) : key;
    rest.replace(':', '-').replace('#', '.');
    return rest;
}

void ChatMemory::load()
{
    for (const QString &filePath : allConversationFilePaths()) {
        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly))
            continue;

        const QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
        f.close();

        const QString key = obj.value(STR_KEY_SESSION_KEY).toString();
        if (key.isEmpty())
            continue;

        ConvHistory h;
        h.summary      = obj.value(STR_KEY_SUMMARY).toString();
        h.lastActiveMs = obj.value(STR_KEY_LAST_ACTIVE_MS).toVariant().toLongLong();

        for (const QJsonValue &v : obj.value(STR_KEY_MESSAGES).toArray()) {
            const QJsonObject m = v.toObject();
            MemoryMessage msg;
            msg.role        = m.value(STR_KEY_ROLE).toString();
            msg.content     = m.value(STR_KEY_CONTENT).toString();
            msg.toolCalls   = m.value(STR_KEY_TOOL_CALLS).toArray();
            msg.toolCallId  = m.value(STR_KEY_TOOL_CALL_ID).toString();
            msg.timestamp   = m.value(STR_KEY_TIMESTAMP).toVariant().toLongLong();
            h.messages.append(msg);
        }

        m_store.insert(key, h);
    }

    qCDebug(logChatBot) << "Loaded" << m_store.size() << "conversations";
}

void ChatMemory::scheduleSave(const QString &key)
{
    m_dirtyKeys.insert(key);
    if (!m_saveTimer.isActive())
        m_saveTimer.start();
}

void ChatMemory::save(const QString &key)
{
    const QString platform = platformFromKey(key);
    if (platform.isEmpty())
        return;

    const QString filePath =
        ChatBotPaths::instance().conversationFile(platform, encodeKey(key));
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    const ConvHistory &h = m_store.value(key);

    QJsonObject obj;
    obj[STR_KEY_SESSION_KEY]    = key;
    obj[STR_KEY_SUMMARY]        = h.summary;
    obj[STR_KEY_LAST_ACTIVE_MS] = h.lastActiveMs;

    QJsonArray messages;
    for (const MemoryMessage &m : h.messages) {
        QJsonObject msg;
        msg[STR_KEY_ROLE]      = m.role;
        msg[STR_KEY_CONTENT]   = m.content;
        msg[STR_KEY_TIMESTAMP] = m.timestamp;
        if (!m.toolCalls.isEmpty())
            msg[STR_KEY_TOOL_CALLS] = m.toolCalls;
        if (!m.toolCallId.isEmpty())
            msg[STR_KEY_TOOL_CALL_ID] = m.toolCallId;
        messages.append(msg);
    }
    obj[STR_KEY_MESSAGES] = messages;

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(logChatBot) << "Failed to save conversation:" << filePath;
        return;
    }
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    f.close();

    qCDebug(logChatBot) << "Saved conversation:" << key;
}

void ChatMemory::tryCompress(const QString &key)
{
    ConvHistory &h = m_store[key];

    // 计算 messages 总字符数
    int total = 0;
    for (const MemoryMessage &m : h.messages)
        total += messageCharCount(m);

    if (total <= MAX_CHARS)
        return;

    const int targetChars = MAX_CHARS / 2;

    // 找到"保护区"起始位置：保留最近 TOOL_RESULT_KEEP_N 轮工具调用不压缩
    // 从后往前数 TOOL_RESULT_KEEP_N 个 user 消息，以此为保护区起点
    int protectStart = h.messages.size();
    int turnsFromBack = 0;
    for (int i = h.messages.size() - 1; i >= 0; --i) {
        if (h.messages[i].role == QLatin1String(STR_KEY_USER)) {
            ++turnsFromBack;
            if (turnsFromBack >= TOOL_RESULT_KEEP_N) {
                protectStart = i;
                break;
            }
        }
    }

    QString compressed;

    // 从头部按完整轮次压缩，直到保护区或字符降至目标值
    while (!h.messages.isEmpty()
           && h.messages.first().role == QLatin1String(STR_KEY_USER)) {
        // 检查当前大小
        int remaining = 0;
        for (int i = 0; i < protectStart && i < h.messages.size(); ++i)
            remaining += messageCharCount(h.messages[i]);
        if (remaining <= targetChars)
            break;

        // 已到达保护区边界则停止
        if (h.messages.first().role == QLatin1String(STR_KEY_USER)
                && h.messages.size() > 0
                && &h.messages.first() == &h.messages[protectStart])
            break;

        // 取出一整轮（user + 后续所有非 user 消息）
        const MemoryMessage userMsg = h.messages.takeFirst();
        --protectStart;
        compressed += QStringLiteral("**用户**: ") + userMsg.content + QStringLiteral("\n\n");

        while (!h.messages.isEmpty()
               && h.messages.first().role != QLatin1String(STR_KEY_USER)) {
            const MemoryMessage m = h.messages.takeFirst();
            --protectStart;
            // 只保留最终 assistant 文字摘要，丢弃 tool_calls / tool result 详情
            if (m.role == QLatin1String(STR_KEY_ASSISTANT) && m.toolCalls.isEmpty()
                    && !m.content.isEmpty())
                compressed += QStringLiteral("**助手**: ") + m.content + QStringLiteral("\n\n");
        }
    }

    if (!compressed.isEmpty()) {
        if (!h.summary.isEmpty())
            h.summary += '\n';
        h.summary += compressed;
        qCDebug(logChatBot) << "Compressed conversation" << key
                           << "summary size:" << h.summary.size()
                           << "remaining messages:" << h.messages.size();
    }
}

QJsonArray ChatMemory::sanitizeToolMessages(const QJsonArray &msgs)
{
    // Pass 1：收集所有有对应 tool result 的 tool_call_id
    QSet<QString> resultIds;
    for (const QJsonValue &v : msgs) {
        const QJsonObject msg = v.toObject();
        if (msg.value(STR_KEY_ROLE).toString() == QLatin1String(STR_KEY_TOOL)) {
            const QString id = msg.value(STR_KEY_TOOL_CALL_ID).toString();
            if (!id.isEmpty())
                resultIds.insert(id);
        }
    }

    // Pass 2：过滤孤立的 tool_calls 条目
    QJsonArray out;
    for (const QJsonValue &v : msgs) {
        QJsonObject msg = v.toObject();
        const QString role = msg.value(STR_KEY_ROLE).toString();

        if (role == QLatin1String(STR_KEY_ASSISTANT) && msg.contains(STR_KEY_TOOL_CALLS)) {
            QJsonArray filteredCalls;
            for (const QJsonValue &tc : msg.value(STR_KEY_TOOL_CALLS).toArray()) {
                const QString id = tc.toObject().value(STR_KEY_ID).toString();
                if (resultIds.contains(id))
                    filteredCalls.append(tc);
            }
            if (filteredCalls.isEmpty()) {
                // 所有 tool_calls 都孤立：若有文本则退化为普通 assistant 消息
                const QString content = msg.value(STR_KEY_CONTENT).toString();
                if (!content.isEmpty()) {
                    msg.remove(STR_KEY_TOOL_CALLS);
                    out.append(msg);
                }
                // 否则整条跳过
                continue;
            }
            msg[STR_KEY_TOOL_CALLS] = filteredCalls;
        }

        out.append(msg);
    }
    return out;
}

int ChatMemory::messageCharCount(const MemoryMessage &m)
{
    int n = m.content.size() + m.toolCallId.size();
    for (const QJsonValue &tc : m.toolCalls)
        n += QJsonDocument(tc.toObject()).toJson(QJsonDocument::Compact).size();
    return n;
}
