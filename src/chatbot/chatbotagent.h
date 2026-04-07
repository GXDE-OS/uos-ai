#pragma once

#include "mcpagent.h"
#include "skillsmanager.h"
#include "tooluse.h"

#include <QScopedPointer>
#include <QSharedPointer>

namespace uos_ai {
namespace chatbot {

inline constexpr char kChatbotAgentName[]           = "chatbot-agent";
inline constexpr char kChatbotHistorySummaryParam[] = "chatbot_history_summary";
inline constexpr char kChatbotPlatformParam[]       = "chatbot_platform";

/**
 * @brief ChatbotAgent - 专为 chatbot 模块设计的 Agent
 *
 * 基于 MCPAgent，支持 MCP 服务器工具、Skill，并保留内置工具（get_current_datetime）。
 * - 工具通过原生 function call API 传递（m_tools 作为 API tools 参数），
 *   不在 system prompt 中注入工具描述文本
 * - 模型以 JSON tool_calls 字段返回工具调用，由 OAIFunctionParser 解析
 * - fetchTools 先拉取 MCP 服务器工具，再追加内置工具和 Skill 工具
 * - update_profile 工具：静默更新 PROFILE.md / SOUL.md，不推送到消息卡片
 */
class ChatbotAgent : public uos_ai::MCPAgent
{
    Q_OBJECT
public:
    explicit ChatbotAgent(QObject *parent = nullptr);

    static QSharedPointer<LlmAgent> create();

    QJsonObject processRequest(const QJsonObject &question, const QJsonArray &history,
                               const QVariantHash &params) override;

    QPair<int, QString> fetchTools(const QStringList &servers) override;

    bool syncServers() const override;

protected:
    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;

private:
    QPair<int, QString> toolGetCurrentDatetime(const QJsonObject &params);
    QPair<int, QString> toolUpdateProfile(const QJsonObject &params);
    QJsonObject         createSkillTool(const QList<uos_ai::SkillInfo> &skillList);
    QPair<int, QString> getSkill(const QString &name);

    /**
     * Bootstrap Hook：若 PROFILE.md 不存在（首次启动），在用户消息前注入 bootstrap
     * 引导文本，指示模型完成身份初始化并写入 PROFILE.md / SOUL.md。
     * PROFILE.md 写入后 update_profile 工具自动触发，下次请求时 hook 不再介入。
     */
    void bootstrapHook(QJsonObject &question) const;

    QString m_platform;           // 当前平台，processRequest 时从 params 注入
    bool    m_isBootstrapSession = false;  // 本次请求是否为首次上线（PROFILE.md 不存在），processRequest 计算一次，fetchTools/bootstrapHook 共用

    QScopedPointer<uos_ai::SkillsManager> m_skills;
};

} // namespace chatbot
} // namespace uos_ai
