#pragma once

#include <QObject>
#include <QHash>
#include <QList>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QTimer>

namespace uos_ai {
namespace chatbot {

struct MemoryMessage {
    QString    role;        // "user" | "assistant" | "tool"
    QString    content;     // text content (empty for assistant-with-tool_calls)
    QJsonArray toolCalls;   // non-empty when role=="assistant" and has tool calls
    QString    toolCallId;  // non-empty when role=="tool"
    qint64     timestamp = 0;
};

struct ConvStats {
    int    messageCount = 0;
    int    charCount    = 0;
    qint64 lastActiveMs = 0;
};

/**
 * @brief ChatMemory - 多会话多轮对话历史（内存 + JSON 持久化）
 *
 * - 会话 Key = platform + ":" + conversationId
 * - 群聊隔离：传入 senderId 时实际 key = platform:conversationId:senderId
 * - 历史完整保存：包含 tool_calls / tool result 消息，供模型跨轮感知工具调用
 * - 压缩策略：每次 assistant 消息写入后检查总字符数；超过 MAX_CHARS 时将最旧的
 *             完整对话轮次转成 markdown 追加到 summary，保留最近 TOOL_RESULT_KEEP_N
 *             轮工具调用不压缩。summary 经 ChatbotAgent::processRequest 注入系统提示词。
 */
class ChatMemory : public QObject
{
    Q_OBJECT

    static constexpr int MAX_CHARS           = 100000; // 单会话消息总字符上限
    static constexpr int TOOL_RESULT_KEEP_N  = 5;      // 压缩时保留最近 N 轮工具调用

public:
    explicit ChatMemory(QObject *parent = nullptr);

    // ── 会话历史 ─────────────────────────────────────────────

    /**
     * 追加一条纯文本消息（user / assistant）；assistant 消息写入后自动触发压缩检查。
     * 用于不经过工具调用的简单场景。
     * @param senderId  非空时启用群聊隔离（实际 key = key:senderId）
     */
    void append(const QString &key, const QString &role, const QString &content,
                const QString &senderId = {});

    /**
     * 存储完整一轮对话：用户消息 + agent context 链（tool_calls + tool results + 最终 assistant）。
     * agentContext 为 LlmAgent::processRequest 返回的 response["context"] QJsonArray，
     * 包含 OAI 格式的 assistant/tool 消息序列。
     * @param senderId  非空时启用群聊隔离
     */
    void appendTurn(const QString &key, const QString &userText,
                    const QJsonArray &agentContext, const QString &senderId = {});

    /**
     * 构建注入上下文的 JSON 数组（OAI 格式）
     * 包含已存储的消息历史（含 tool_calls / tool result）和当前用户输入。
     * 输出前会对工具消息做合法性校验（sanitizeToolMessages）。
     */
    QJsonArray buildContext(const QString &key, const QString &userText,
                            const QString &senderId = {}) const;

    /**
     * 删除指定会话最后一条指定角色的消息（重试 / 错误撤销用）
     */
    bool removeLast(const QString &key, const QString &role,
                    const QString &senderId = {});

    /** 查询会话统计信息 */
    ConvStats stats(const QString &key, const QString &senderId = {}) const;

    /** 清空指定会话 */
    void clearConversation(const QString &key, const QString &senderId = {});

    /** 清空所有会话（内存 + 磁盘） */
    void clearAll();

    /** 返回当前所有活跃会话的 key 列表 */
    QStringList activeKeys() const;

    /** 返回指定会话已压缩的历史摘要（markdown 格式），无则返回空字符串 */
    QString summaryFor(const QString &key, const QString &senderId = {}) const;

private:
    static constexpr int SAVE_DELAY_MS = 2000;

    QString resolveKey(const QString &key, const QString &senderId) const;
    QString encodeKey(const QString &key) const;
    QStringList allConversationFilePaths() const;

    void load();
    void scheduleSave(const QString &key);
    void save(const QString &key);

    /**
     * 将超出字符预算的最旧完整轮次转成 markdown 追加到 summary，
     * 保留最近 TOOL_RESULT_KEEP_N 轮工具调用。
     */
    void tryCompress(const QString &key);

    /**
     * 过滤历史消息中未配对的 tool_calls / tool result，防止孤立消息导致 API 报错。
     */
    static QJsonArray sanitizeToolMessages(const QJsonArray &msgs);

    /** 估算消息字符数（含 toolCalls JSON 内容） */
    static int messageCharCount(const MemoryMessage &m);

    struct ConvHistory {
        QList<MemoryMessage> messages;
        qint64  lastActiveMs = 0;
        QString summary;  // 已压缩的旧消息，markdown 格式，注入到 system prompt
    };

    QHash<QString, ConvHistory> m_store;
    QSet<QString>               m_dirtyKeys;
    QTimer                      m_saveTimer;
};

} // namespace chatbot
} // namespace uos_ai
