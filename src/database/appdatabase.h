#ifndef APPDATABASE_H
#define APPDATABASE_H

#include "global_define.h"
#include "sqlitebase.h"
#include "modelinfo.h"

#include <QString>
#include <QList>

namespace uos_ai {

inline constexpr char CONFIG_WINDOW_MODE[] = "window_mode";
inline constexpr char CONFIG_MAIN_WINDOW_SIZE[] = "main_window_size";
inline constexpr char CONFIG_MAIN_WINDOW_SIDEBAR_WIDTH[] = "main_window_sidebar_width";
inline constexpr char CONFIG_MAIN_WINDOW_SIDEBAR_EXPANDED[] = "main_window_sidebar_expanded";
inline constexpr char CONFIG_MAIN_WINDOW_SIDEBAR_GROUP_COLLAPSED_STATES[] =
    "main_window_sidebar_group_collapsed_states";
inline constexpr char CONFIG_THIRD_PARTY_MCP[] = "third_party_mcp";
inline constexpr char CONFIG_APP_AGREEMENT[] = "app_agreement";
inline constexpr char CONFIG_ASSISTANT_ORDER[] = "assistant_order";
// 助手侧边栏常驻显示数量，用于保存跨常驻区/隐藏区拖拽后的布局。
inline constexpr char CONFIG_ASSISTANT_VISIBLE_COUNT[] = "assistant_visible_count";
inline constexpr char CONFIG_EXTERNAL_MODELS[] = "external_models";
// 已展示的新手引导枚举版本。
inline constexpr char CONFIG_NEW_USER_GUIDE_SHOWN_VERSION[] = "new_user_guide_shown_version";
// 已消费的 UOS AI 应用更新提醒版本。值为归一化后的版本号，例如 1.2.3.4。
inline constexpr char CONFIG_APP_UPDATE_LAST_CONSUMED_VERSION[] =
    "app_update_last_consumed_version";

class AppDatabase : public SQLiteBase
{
public:
    static AppDatabase *instance();

    bool init();

    QString getDatabasePath() const;

    QMap<QString, ProviderAccount> queryAllProviders();
    QList<ModelAccountPtr> queryAllModels();

    void deleteProvider(const QString &id);
    void deleteModel(const QString &id);
    
    void saveConfig(const QString &key, const QString &value);
    QString getConfig(const QString &key);

    inline void saveConfigBool(const QString &key, bool value) {
        saveConfig(key, QString(value ? "1" : "0"));
    }

    inline bool getConfigBool(const QString &key) {
        return getConfig(key).toInt();
    }

    inline void saveConfigInt(const QString &key, int value) {
        saveConfig(key, QString::number(value));
    }

    inline int getConfigInt(const QString &key) {
        return getConfig(key).toInt();
    }

private:
    AppDatabase();
    ~AppDatabase();

    bool createTables();

    QString m_dbPath;
    QString m_dbName;
};

}

#endif // APPDATABASE_H
