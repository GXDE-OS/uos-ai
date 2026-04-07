#pragma once

#include "abstractchannel.h"

#include <QWebSocket>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QVector>

namespace uos_ai {
namespace chatbot {

/**
 * @brief FeishuChannel - 飞书/Lark 机器人频道（WebSocket 长连接）
 *
 * 接收：WebSocket 二进制帧（pbbp2 Protobuf 协议）
 *   POST /callback/ws/endpoint → 获取 wss:// URL
 *   收到 DATA/EVENT 帧 → 解析事件 → 发 ACK（Protobuf）
 *   收到 CONTROL/PING → 忽略（Qt WS 层自动 Pong）
 *
 * 发送：HTTPS REST API（open.feishu.cn）
 *   POST /open-apis/auth/v3/tenant_access_token/internal → tenant_access_token
 *   POST /open-apis/im/v1/messages                       → 发送消息
 *
 * 注意：
 *   - 飞书 WS 帧为二进制 Protobuf，连接 binaryMessageReceived 信号
 *   - 大消息分片：sum>1 时需缓存各片，全部到达后合并
 *   - 消息去重：基于 message_id（飞书 at-least-once 投递）
 */
class FeishuChannel : public AbstractChannel
{
    Q_OBJECT

public:
    explicit FeishuChannel(QObject *parent = nullptr);

    void    start(const QJsonObject &config) override;
    void    stop() override;
    void    sendMessage(const QString &to, const QString &content,
                        const QString &conversationType) override;
    bool    isRunning() const override { return m_running; }
    QString platformName() const override { return QStringLiteral("feishu"); }

    QString beginStreamingReply(const QString &to,
                                const QString &conversationType) override;
    void    updateStreamingReply(const QString &streamHandle,
                                 const QString &content) override;
    void    finalizeStreamingReply(const QString &streamHandle) override;

private Q_SLOTS:
    void onWsConnected();
    void onWsDisconnected();
    void onWsBinaryMessageReceived(const QByteArray &data);
    void onWsError(QAbstractSocket::SocketError error);
    void sendPing();
    void cleanupStaleFragments();

private:
    // --- 连接流程 ---
    void fetchEndpoint();
    void scheduleReconnect();

    // --- 帧处理（内部用原始 bytes 避免 protobuf 头文件泄漏到 .h）---
    void handleBinaryFrame(const QByteArray &data);
    void handleDataFrame(const QByteArray &frameBytes, const QByteArray &payload,
                         const QString &msgId, int sum, int seq, const QString &type);
    void handleEventPayload(const QByteArray &frameBytes, const QByteArray &payload);
    QByteArray assemblePayload(const QString &msgId, int sum, int seq,
                               const QByteArray &chunk);

    // --- ACK ---
    void sendAck(const QByteArray &originalFrameBytes, int code = 200);

    // --- Ping ---
    void sendProtobufPing();

    // --- Token ---
    void ensureToken(std::function<void()> action);
    void fetchToken();
    void flushPendingTokenActions();

    // --- 发送 ---
    void doSendMessage(const QString &to, const QString &content,
                       const QString &conversationType);

    // --- 流式卡片 ---
    void doCreateStreamingCard(const QString &handle);
    void doSendCardMessage(const QString &handle);
    void doUpdateCardContent(const QString &handle, const QString &content);
    void doFinalizeCard(const QString &handle);

    struct StreamingReply {
        QString cardId;           // 卡片创建成功后填入；空表示仍在创建中
        QString to;
        QString conversationType;
        QString pendingContent;   // 卡片创建完成前到达的内容
        bool    pendingFinalize = false; // finalize 在卡片创建前、或内容更新进行中时到达
        bool    failed = false;   // 卡片创建失败，降级为普通消息
        int     sequence = 0;     // 单调递增，防止乱序更新
        int     inflightUpdates = 0; // 进行中的内容更新请求数；>0 时 finalize 需等待
    };
    QMap<QString, StreamingReply> m_streamingReplies;

    // --- 凭据 ---
    QString m_appId;
    QString m_appSecret;
    QString m_domain;           // "https://open.feishu.cn" 或 lark

    // --- Token ---
    QString m_accessToken;
    qint64  m_tokenExpiry = 0;
    enum class TokenState { Invalid, Refreshing, Valid };
    TokenState m_tokenState = TokenState::Invalid;
    QList<std::function<void()>> m_pendingActions;

    // --- WS 参数（从 ClientConfig 动态更新）---
    int m_pingIntervalMs  = 120000;  // 120s
    int m_serviceId       = 0;

    // --- 消息分片缓存 ---
    struct FragmentCache {
        QVector<QByteArray> parts;  // indexed by seq (0-based)
        qint64 createdAt;           // Unix 秒，用于超时清理
    };
    QMap<QString, FragmentCache> m_fragments;  // msg_id → cache

    // --- 消息去重 ---
    QSet<QString> m_processedMsgIds;    // 已处理的 message_id
    QTimer       *m_dedupeCleanTimer;   // 定期清理（30 min）

    // --- 状态 ---
    bool m_running = false;

    QWebSocket            *m_ws = nullptr;
    QNetworkAccessManager *m_http = nullptr;
    QTimer                *m_pingTimer = nullptr;
    QTimer                *m_reconnectTimer = nullptr;
    QTimer                *m_fragmentCleanTimer = nullptr;

    static constexpr int kReconnectInterval      = 5000;   // ms
    static constexpr int kFragmentTtlSecs        = 30;     // 分片超时
    static constexpr int kDedupeCleanIntervalMs  = 1800000; // 30 min

    // --- API 路径 ---
    static const char *kEndpointPath;
    static const char *kTokenPath;
    static const char *kSendMsgPath;
    static const char *kCreateCardPath;      // POST   创建卡片实体
    static const char *kCardElementFmt;      // PATCH  流式更新文本（含 %1=card_id）
    static const char *kCardSettingsFmt;     // PATCH  更新卡片配置（含 %1=card_id）

    // 流式卡片卡片内文本组件 ID（固定值，与创建卡片时写入的 element_id 一致）
    static const char *kStreamElementId;
};

} // namespace chatbot
} // namespace uos_ai
