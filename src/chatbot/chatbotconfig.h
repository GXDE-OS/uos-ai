#pragma once

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <optional>

namespace uos_ai {
namespace chatbot {

/**
 * @brief ChannelConfig - 单个 IM 平台的配置
 *
 * 保留原始 JSON（传给 AbstractChannel::start()），同时提供类型化访问器，
 * 消除散落各处的 .value("key").toString() 手写解析。
 */
struct ChannelConfig {
    bool    enabled  = false;
    QString platform;

    // ── 类型化访问器（不同平台按需使用） ──────────────────────
    QString appId()        const;   // 飞书 / QQ
    QString appSecret()    const;   // 飞书
    QString clientId()     const;   // 钉钉
    QString clientSecret() const;   // 钉钉
    QString secret()       const;   // QQ

    /// 返回适合传入 AbstractChannel::start() 的原始对象
    const QJsonObject &raw() const { return m_raw; }

    static ChannelConfig fromJson(const QString &platformName, const QJsonObject &obj);

private:
    QJsonObject m_raw;
};

/**
 * @brief ChatBotConfig - chatbot.json 的完整配置模型
 *
 * 统一解析入口，集中处理字段默认值和格式兼容性。
 * 使用 std::optional 返回值：解析失败时附带错误信息，方便日志定位。
 */
struct ChatBotConfig {
    bool enabled = false;
    QMap<QString, ChannelConfig> platforms;

    bool isEmpty() const { return !enabled && platforms.isEmpty(); }

    /**
     * @brief 从 JSON 解析
     * @param obj   chatbot.json 的根对象
     * @param error 非空时写入失败原因
     * @return 成功返回 ChatBotConfig；JSON 为空时返回 nullopt
     */
    static std::optional<ChatBotConfig> fromJson(const QJsonObject &obj,
                                                  QString *error = nullptr);

    QJsonObject toJson() const;
};

} // namespace chatbot
} // namespace uos_ai
