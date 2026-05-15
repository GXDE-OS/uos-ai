#ifndef CHATBOTSERVICE_H
#define CHATBOTSERVICE_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QJsonObject>
#include <optional>

#include "chatbotconfig.h"

namespace uos_ai {
namespace chatbot {

class ChannelManager;
class ChatBotRequestHandler;

/**
 * @brief ChatBotService - chatbot 功能统一入口
 *
 * 职责：
 * 1. 读取 chatbot.json 配置文件（enabled/platforms）
 * 2. 创建并管理 ChannelManager、ChatBotRequestHandler
 * 3. 监听配置文件变更，热重载各平台 Channel
 * 4. 向外提供 applyConfig / config 接口供设置界面调用
 *
 * 配置文件路径：~/.config/deepin/uos-ai-assistant/chatbot.json
 * 格式：
 * {
 *   "enabled": true,
 *   "platforms": {
 *     "feishu":   { "enabled": true, "app_id": "...", "app_secret": "..." },
 *     "dingtalk": { "enabled": false, "client_id": "...", "client_secret": "..." },
 *     "qq":       { "enabled": false, "app_id": "...", "secret": "..." }
 *   }
 * }
 */
class ChatBotService : public QObject
{
    Q_OBJECT

public:
    static ChatBotService *instance();

    /// 完成初始化：绑定 channel manager、启动配置文件监听、加载首次配置
    void initialize();

    /// 更新平台配置并立即应用
    void applyConfig(const QJsonObject &config);

    /// 返回当前配置
    QJsonObject config() const;

Q_SIGNALS:
    /// 配置加载或更新后发出（供设置界面同步显示）
    void configChanged(const QJsonObject &config);

private Q_SLOTS:
    void onConfigFileChanged(const QString &path);

private:
    void        loadAndApply();
    QJsonObject readConfigFile() const;

    ChatBotService();
    ~ChatBotService();

    ChannelManager       *m_channelManager = nullptr;
    ChatBotRequestHandler *m_handler       = nullptr;

    QJsonObject m_config;
    std::optional<ChatBotConfig> m_lastValidConfig;   // 上一次成功应用的配置，用于热重载失败时回滚
    mutable QFileSystemWatcher m_watcher;
};

} // namespace chatbot
} // namespace uos_ai

#endif // CHATBOTSERVICE_H
