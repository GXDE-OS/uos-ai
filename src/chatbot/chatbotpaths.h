#pragma once

#include <QString>

namespace uos_ai {
namespace chatbot {

/**
 * @brief ChatBotPaths - chatbot 所有文件路径的集中管理单例
 *
 * 职责：
 * - 统一定义配置文件、数据目录的路径，替代各模块中散落的硬编码路径
 * - 支持通过 setOverrideDataDir() 覆盖数据目录（测试隔离用）
 *
 * 使用方式：
 *   ChatBotPaths::instance().configFile()
 */
class ChatBotPaths
{
public:
    static ChatBotPaths &instance();

    /// 覆盖数据目录（主要用于测试隔离），传空字符串恢复默认
    void setOverrideDataDir(const QString &dir);

    // ── 配置路径 ──────────────────────────────────────────────
    /// ~/.config/deepin/uos-ai-assistant/chatbot.json
    QString configFile() const;
    /// 配置文件所在目录
    QString configDir()  const;

    // ── 数据路径 ──────────────────────────────────────────────
    /// 数据根目录：~/.local/share/uos-ai-assistant/chatbot/
    QString dataDir() const;
    /// 用户画像文件：<dataDir>/memory.json
    QString profileFile() const;
    /// 会话文件目录：<dataDir>/<platform>/conversations/
    QString conversationsDir(const QString &platform) const;
    /// 会话文件路径：<conversationsDir(platform)>/<encodedKey>.json
    QString conversationFile(const QString &platform, const QString &encodedKey) const;

    // ── Bootstart 工作区文件（每个平台独立） ─────────────────
    /// <dataDir>/<platform>/PROFILE.md — 存在则表示该平台 bootstart 已完成
    QString profileMdFile(const QString &platform) const;
    /// <dataDir>/<platform>/SOUL.md
    QString soulMdFile(const QString &platform)    const;

private:
    ChatBotPaths() = default;
    QString m_overrideDataDir;
};

} // namespace chatbot
} // namespace uos_ai
