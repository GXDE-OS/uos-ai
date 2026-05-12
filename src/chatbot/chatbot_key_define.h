#ifndef CHATBOT_KEY_DEFINE_H
#define CHATBOT_KEY_DEFINE_H

namespace uos_ai {

// Canonical message payload (AbstractChannel unified format)
inline constexpr char STR_KEY_PLATFORM[]        = "platform";
inline constexpr char STR_KEY_MESSAGE_ID[]      = "message_id";
inline constexpr char STR_KEY_SENDER[]          = "sender";
inline constexpr char STR_KEY_CONVERSATION[]    = "conversation";
inline constexpr char STR_KEY_TIMESTAMP[]       = "timestamp";

// ChatMemory persistence
inline constexpr char STR_KEY_SESSION_KEY[]     = "sessionKey";
inline constexpr char STR_KEY_LAST_ACTIVE_MS[]  = "lastActiveMs";

// Config file structure
inline constexpr char STR_KEY_ENABLED[]         = "enabled";
inline constexpr char STR_KEY_PLATFORMS[]       = "platforms";

// Channel configuration
inline constexpr char STR_KEY_APP_SECRET[]      = "app_secret";
inline constexpr char STR_KEY_CLIENT_ID[]       = "client_id";
inline constexpr char STR_KEY_CLIENT_SECRET[]   = "client_secret";
inline constexpr char STR_KEY_SECRET[]          = "secret";
inline constexpr char STR_KEY_CARD_TEMPLATE_ID[] = "card_template_id";
inline constexpr char STR_KEY_TOKEN[]           = "token";
inline constexpr char STR_KEY_DOMAIN[]          = "domain";
inline constexpr char STR_KEY_BOT_TOKEN[]       = "bot_token";
inline constexpr char STR_KEY_APPLICATION_ID[]  = "application_id";
inline constexpr char STR_KEY_GUILD_ID[]        = "guild_id";
inline constexpr char STR_KEY_API_BASE[]        = "api_base";
inline constexpr char STR_KEY_META[]            = "meta";

// Common API response
inline constexpr char STR_KEY_ACCESS_TOKEN[]    = "access_token";
inline constexpr char STR_KEY_EXPIRES_IN[]      = "expires_in";
inline constexpr char STR_KEY_CODE[]            = "code";

// Session params
inline constexpr char STR_KEY_MCP_SERVERS[]     = "mcpServers";

// Platform identifiers
inline constexpr char STR_PLATFORM_FEISHU[]     = "feishu";
inline constexpr char STR_PLATFORM_LARK[]       = "lark";
inline constexpr char STR_PLATFORM_DINGTALK[]   = "dingtalk";
inline constexpr char STR_PLATFORM_QQ[]         = "qq";
inline constexpr char STR_PLATFORM_TELEGRAM[]   = "telegram";
inline constexpr char STR_PLATFORM_DISCORD[]    = "discord";

} // namespace uos_ai

#endif // CHATBOT_KEY_DEFINE_H
