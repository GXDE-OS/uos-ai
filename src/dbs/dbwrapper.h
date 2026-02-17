#ifndef DBWRAPPER_H
#define DBWRAPPER_H

#include "serverdefs.h"
#include "appdefs.h"

#include <QPointer>
#include <QStandardPaths>
#include <QMap>
#include <QDir>

class DaoClient;

class LlmTable;

enum CopilotDbType {
    COPILOT_BASIC       = 0,  // 基础信息数据库
    COPILOT_CHATINFO    = 1,  // 聊天记录数据库
};

enum UpdatePromptBitType {
    SHORTCUT_UPDATE = (0 << 8) | 2,    // 快捷键更新弹窗控制位（第0位开始，2位长度）
    AGENT_UPDATE = (2 << 8) | 1,       // 智能体更新弹窗控制位（第2位开始，1位长度）
    MCP_UPDATE = (3 << 8) | 1,         // MCP更新弹窗控制位（第3位开始，1位长度）
    PRIVACY_UPDATE = (4 << 8) | 1,      // 隐私对话更新弹窗控制位（第4位开始，1位长度）
    AUTO_MCP = (5 << 8) | 1,      // MCP升级为自动模式（第5位开始，1位长度）
    FREE_CREDITS = (6 << 8) | 1,      // 免费额度（第6位开始，1位长度）
};

// 辅助函数来提取位配置
inline int getStartBit(UpdatePromptBitType type) { return (type >> 8) & 0xFF; }
inline int getLength(UpdatePromptBitType type) { return type & 0xFF; }

class DbWrapper
{
public:
    static inline int builtinGuideVersion() {
        return  1;
    }
    static inline int maxShowUpdatePromptTimes() {
        return  3;
    }
    static DbWrapper &localDbWrapper();

    ~DbWrapper();

    int initialization(const QString &dir);

    bool addColumnIfNotExists(const QString &tableName, const QString &columnName, const QString &dataType);

    bool initAssistant();

    bool updateOldAssistantId();

    void checkAndAddMissingAssistants();

    // 大模型表操作
    bool appendLlm(const LLMServerProxy &llmServerProxy);

    bool deleteLlm(const QString &lmid);

    bool updateLlm(const LLMServerProxy &llmServerProxy);

    LLMServerProxy queryLlmByLlmid(const QString &lmid);

    QList<LLMServerProxy> queryLlmList(bool all = false);

    // 助手角色表
    bool appendAssistant(const AssistantProxy &assistantProxy);

    bool deleteAssistant(const QString &assistantId);

    bool updateAssistant(const AssistantProxy &assistantProxy);

    bool updateAssistantId(const AssistantProxy &assistantProxy, const QString &newId);

    AssistantProxy queryAssistantByid(const QString &assistantId);

    QList<AssistantProxy> queryAssistantList(bool all = false);

    // 助手当前选择的模型
    bool appendAssistantLlm(const QString &assistantId, const QString &llmId);

    bool deleteAssistantLlm(const QString &assistantId);

    bool updateAssistantLlm(const QString &assistantId, const QString &llmId);

    QString queryLlmIdByAssistantId(const QString &assistantId);

    // App表的操作
    bool appendApp(const AppDbusPathObject &appObject);

    bool deleteApp(const QString &appid);

    bool updateApp(const AppDbusPathObject &appObject);

    bool updateAppCurllmId(const QString &appid, const QString &llmId);

    bool updateAppCurAssistantId(const QString &appid, const QString &assistantId);

    AppDbusPathObject queryAppByAppId(const QString &appid);

    QString queryCurLlmIdByAppId(const QString &appid);

    QString queryCurAssistantIdByAppId(const QString &appid);

    QMap<QString, AppDbusPathObject> queryAppList();

    // Config 配置表

    QString getVersion();

    bool updateVersion(const QString &version);

    // 返回主题(Light、Mirage、Dark)
    QString getCopilotTheme();

    // 设置主题(Light、Mirage、Dark)
    bool updateCopilotTheme(const QString &newTheme);

    // 获取小助手状态
    bool getAICopilotIsOpen();

    // 更新小助手开关状态 1，开 ； 0，关
    bool updateAICopilot(bool isOpen);

    // 获取第三方MCP协议同意状态
    bool getThirdPartyMcpAgreement();

    // 更新第三方MCP协议同意状态 1，同意 ； 0，不同意
    bool updateThirdPartyMcpAgreement(bool isAgree);

    // 获取用户体验计划开关状态
    int getUserExpState();

    // 更新用户体验计划开关状态 0:未操作过； 小于0:关闭； 大于0:开启
    bool updateUserExpState(int state);

    // 获取本地语音模型开关
    bool getLocalSpeech();

    // 更新本地语音模型开关
    bool updateLocalSpeech(bool isOpen);

    // 窗口模式
    int getDisplayMode();
    bool updateDisplayMode(int );

    // 窗口尺寸
    QString getWindowSize();
    bool updateWindowSize(QString size);

    // DeepSeek领取引导
    QString getGuideKey();
    bool updateGuideKey(const QString &key);

    // 更新提示
    int getUpdatePromptBits(int startBit, int length);
    bool updateUpdatePromptBits(int startBit, int length, int value);
    int getUpdatePromptBits(UpdatePromptBitType type);
    bool updateUpdatePromptBits(UpdatePromptBitType type, int value);

    // 其他操作

    static QString getDatabaseDir();

    static QString getAppDataDir(const QString &subDir);

private:
    DbWrapper();

private:
    DaoClient *m_daoClient;

    static QMap<CopilotDbType, QString> m_qMapDatabases;
    static const QSet<AssistantType> m_allowedTypes;

};

#endif // DBWRAPPER_H
