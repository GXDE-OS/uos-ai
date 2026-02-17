#include "dbwrapper.h"
#include "daoclient.h"
#include "tables/apptable.h"
#include "tables/llmtable.h"
#include "tables/configtable.h"
#include "tables/assistanttable.h"
#include "tables/curllmtable.h"
#include "llmutils.h"
#include "utils/util.h"

#include <QJsonDocument>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logDBS)

static QString gVersion = "Version_1_0";

QMap<CopilotDbType, QString> DbWrapper::m_qMapDatabases = {
    {CopilotDbType::COPILOT_BASIC,       QString("basic")},
    {CopilotDbType::COPILOT_CHATINFO,    QString("chatinfo")}
};

const QSet<AssistantType> DbWrapper::m_allowedTypes = {
    UOS_AI,
    AI_WRITING,
    AI_TEXT_PROCESSING,
    AI_TRANSLATION
};

QString DbWrapper::getDatabaseDir()
{
    return getAppDataDir("db");
}

QString DbWrapper::getAppDataDir(const QString &subDir)
{
    QDir approot(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + subDir);
    if (approot.exists() == false) {
        approot.mkpath(approot.path());
    }
    return approot.path();
}

DbWrapper &DbWrapper::localDbWrapper()
{
    static DbWrapper dbWrapper;
    return dbWrapper;
}

DbWrapper::DbWrapper()
    : m_daoClient(nullptr)
{

}

DbWrapper::~DbWrapper()
{
    if (m_daoClient) {
        m_daoClient->free();
    }
}

int DbWrapper::initialization(const QString &dir)
{
    if (m_daoClient != nullptr)  // 防止重复初始化
        return 0;

    m_daoClient = &DaoClient::getInstance();
    if (m_daoClient == nullptr) {
        qCCritical(logDBS) << "Failed to get DaoClient instance";
        return -1;
    }

    // 创建基础信息数据库
    m_daoClient->add(dir, m_qMapDatabases[COPILOT_BASIC], m_qMapDatabases[COPILOT_BASIC]);
    // 创建聊天记录数据库
    m_daoClient->add(dir, m_qMapDatabases[COPILOT_CHATINFO], m_qMapDatabases[COPILOT_CHATINFO]);

    QString strAppInfo =
            "CREATE TABLE IF NOT EXISTS app ( \
            uuid VARCHAR(64) UNIQUE PRIMARY KEY,\
            name VARCHAR(128),\
            desc VARCHAR(512), \
            llmid VARCHAR(128), \
            cmd TEXT,\
            ext TEXT\
            );";
    QString strLlmInfo =
            "CREATE TABLE IF NOT EXISTS llm (\
            uuid VARCHAR(64) UNIQUE PRIMARY KEY,\
            name VARCHAR(128),\
            type INTEGER, \
            desc VARCHAR(512),\
            account_proxy TEXT,\
            ext TEXT\
            );";
    QString strConfigInfo =
            "CREATE TABLE IF NOT EXISTS config (\
            id INTEGER PRIMARY KEY AUTOINCREMENT,\
            name VARCHAR(128),\
            type INTEGER UNIQUE,\
            desc VARCHAR(512),\
            value TEXT\
            );";

    QString strAssistantInfo =
            "CREATE TABLE IF NOT EXISTS assistant (\
            id VARCHAR(128) UNIQUE PRIMARY KEY,\
            display_name VARCHAR(128),\
            type INTEGER, \
            description VARCHAR(512)\
            );";

    QString strAssistantLlmInfo =
            "CREATE TABLE IF NOT EXISTS curllm (\
            assistantid VARCHAR(128) UNIQUE PRIMARY KEY,\
            llmid VARCHAR(128)\
            );";

    DaoResultListPtr result;
    QString message;

    if (!m_daoClient->execSync(strAppInfo, result, message,  m_qMapDatabases[COPILOT_BASIC])) {
        qCWarning(logDBS) << "Failed to create app table:" << message;
    }
    if (!m_daoClient->execSync(strLlmInfo, result, message,  m_qMapDatabases[COPILOT_BASIC])) {
        qCWarning(logDBS) << "Failed to create llm table:" << message;
    }
    if (!m_daoClient->execSync(strConfigInfo, result, message,  m_qMapDatabases[COPILOT_BASIC])) {
        qCWarning(logDBS) << "Failed to create config table:" << message;
    }
    if (!m_daoClient->execSync(strAssistantInfo, result, message,  m_qMapDatabases[COPILOT_BASIC])) {
        qCWarning(logDBS) << "Failed to create assistant table:" << message;
    }
    if (!m_daoClient->execSync(strAssistantLlmInfo, result, message,  m_qMapDatabases[COPILOT_BASIC])) {
        qCWarning(logDBS) << "Failed to create curllm table:" << message;
    }

    addColumnIfNotExists("app", "assistantid", "INTEGER"); //添加新列，记录助手id

    if (queryAssistantList().isEmpty()) {
        qCInfo(logDBS) << "Initializing default assistants...";
        initAssistant(); //初始化固定角色信息
    } else {
        qCInfo(logDBS) << "Updating existing assistant IDs...";
        updateOldAssistantId();
        checkAndAddMissingAssistants();
    }

    const auto &version = getVersion();
    if (version.isEmpty()) {
        qCInfo(logDBS) << "Setting initial version:" << gVersion;
        updateVersion(gVersion);
        updateCopilotTheme("Light");
        updateAICopilot(false);
        updateThirdPartyMcpAgreement(false);  // 默认不同意使用第三方MCP协议
        //0:侧边栏模式 1：窗口模式
        updateDisplayMode(1);
    }

    return 0;
}

bool DbWrapper::addColumnIfNotExists(const QString &tableName, const QString &columnName, const QString &dataType)
{
    DaoResultListPtr result;
    QString message;
    bool columnExists = false;
    bool success = false;

    QString pragma = "PRAGMA table_info(" + tableName + ");";
    if (!m_daoClient->execSync(pragma, result, message,  m_qMapDatabases[COPILOT_BASIC])) {
        qCWarning(logDBS) << "Failed to check column existence:" << message;
    } else {
        for (int i = 0; i < result->length(); i++) {
            if (columnName == result->value(i).value("name").toString()) {
                columnExists = true;
                break;
            }
        }
    }

    if (!columnExists) {
        QString addColumn = "ALTER TABLE " + tableName + " ADD COLUMN " + columnName + " " + dataType + ";";
        if (!m_daoClient->execSync(addColumn, result, message,  m_qMapDatabases[COPILOT_BASIC])) {
            qCWarning(logDBS) << "Failed to add column:" << message;
        } else {
            qCDebug(logDBS) << "Added column" << columnName << "to table" << tableName;
            success = true;
        }
    }

    return success;
}

bool DbWrapper::initAssistant()
{
    AssistantProxy proxyUosAI, proxySystemAssistant, proxyAIWrite, proxyAITextProcessing, proxyAITranslation;
    proxyUosAI.displayName = "UOS AI";
    proxyUosAI.id = Util::generateAssistantUuid(proxyUosAI.displayName);
    proxyUosAI.type = UOS_AI;
    proxyUosAI.description = "System's Comprehensive AI Assistant.";

    // Add AI WordWizard assistant
    proxyAIWrite.displayName = "AI Writing";
    proxyAIWrite.id = Util::generateAssistantUuid(proxyAIWrite.displayName);
    proxyAIWrite.type = AI_WRITING;
    proxyAIWrite.description = "Write Based on Your Topic and Requirements.";

    proxyAITextProcessing.displayName = "AI Text Processing";
    proxyAITextProcessing.id = Util::generateAssistantUuid(proxyAITextProcessing.displayName);
    proxyAITextProcessing.type = AI_TEXT_PROCESSING;
    proxyAITextProcessing.description = "Capable of Handling Text Processing Tasks Such as Summarizing, Proofreading, and Rewriting.";

    proxyAITranslation.displayName = "AI Translation";
    proxyAITranslation.id = Util::generateAssistantUuid(proxyAITranslation.displayName);
    proxyAITranslation.type = AI_TRANSLATION;
    proxyAITranslation.description = "Your Translation Assistant, Mastering Multiple Languages.";

    if (Util::isCommunity()) {
        proxySystemAssistant.displayName = "Deepin System Assistant";
        proxySystemAssistant.id = Util::generateAssistantUuid(proxySystemAssistant.displayName);
        proxySystemAssistant.type = DEEPIN_SYSTEM_ASSISTANT;
        proxySystemAssistant.description = "Assists you with Deepin system-related inquiries";
    } else {
        proxySystemAssistant.displayName = "UOS System Assistant";
        proxySystemAssistant.id = Util::generateAssistantUuid(proxySystemAssistant.displayName);
        proxySystemAssistant.type = UOS_SYSTEM_ASSISTANT;
        proxySystemAssistant.description = "Assists you with UOS system-related inquiries.";
    }
#ifdef ENABLE_PERSONAL_KNOWLEDGE_ASSISTANT
    AssistantProxy proxyPerKnowAssistant;
    proxyPerKnowAssistant.displayName = "Personal Knowledge Assistant";
    proxyPerKnowAssistant.id = Util::generateAssistantUuid(proxyPerKnowAssistant.displayName);
    proxyPerKnowAssistant.type = PERSONAL_KNOWLEDGE_ASSISTANT;
    proxyPerKnowAssistant.description = "Answers questions based on your personal knowledge base.";
#endif

    bool ret = appendAssistant(proxyUosAI);
    ret = appendAssistant(proxySystemAssistant);
#ifdef ENABLE_PERSONAL_KNOWLEDGE_ASSISTANT
    ret = appendAssistant(proxyPerKnowAssistant);
#endif
    ret = appendAssistant(proxyAIWrite);
    ret = appendAssistant(proxyAITextProcessing);
    ret = appendAssistant(proxyAITranslation);

    if (ret) {
        qCInfo(logDBS) << "Default assistants initialized successfully";
    } else {
        qCWarning(logDBS) << "Failed to initialize some default assistants";
    }
    return ret;
}

bool DbWrapper::updateOldAssistantId()
{
    QList<AssistantProxy> assistantList = queryAssistantList();
    bool ret = true;

    QString uosAiPrefix = Util::generateAssistantUuid("UOS AI") + "_";
    QString deepinPrefix = Util::generateAssistantUuid("Deepin System Assistant") + "_";
    QString uosSystemPrefix = Util::generateAssistantUuid("UOS System Assistant") + "_";
    QString personalPrefix = Util::generateAssistantUuid("Personal Knowledge Assistant") + "_";
    QString aiWritePrefix = Util::generateAssistantUuid("AI Write") + "_";
    QString aiTextProcessingPrefix = Util::generateAssistantUuid("AI Text Processing") + "_";
    QString aiTranslatePrefix = Util::generateAssistantUuid("AI Translate") + "_";

    for (const auto &assistant : assistantList) {
        QString id = assistant.id;
        QString newId;
        
        if (id.startsWith(uosAiPrefix)) {
            newId = Util::generateAssistantUuid("UOS AI");
        } else if (id.startsWith(deepinPrefix)) {
            newId = Util::generateAssistantUuid("Deepin System Assistant");
        } else if (id.startsWith(uosSystemPrefix)) {
            newId = Util::generateAssistantUuid("UOS System Assistant");
        } else if (id.startsWith(personalPrefix)) {
            newId = Util::generateAssistantUuid("Personal Knowledge Assistant");
        } else if (id.startsWith(aiWritePrefix)) {
            newId = Util::generateAssistantUuid("AI Write");
        } else if (id.startsWith(aiTextProcessingPrefix)) {
            newId = Util::generateAssistantUuid("AI Text Processing");
        } else if (id.startsWith(aiTranslatePrefix)) {
            newId = Util::generateAssistantUuid("AI Translate");
        }

        if (!newId.isEmpty()) {
            AssistantProxy updatedAssistant = assistant;
            ret = ret && updateAssistantId(updatedAssistant, newId);
            qCInfo(logDBS) << "Updated assistant ID from" << id << "to" << newId;
        }
    }

    return ret;
}

void DbWrapper::checkAndAddMissingAssistants()
{
    QList<AssistantProxy> assistantList = queryAssistantList();
    QSet<AssistantType> existingTypes;
    for (const auto &assistant : assistantList) {
        existingTypes.insert(assistant.type);
    }

    // Define all assistant types in order of their enum values
    struct AssistantInfo {
        AssistantType type;
        QString displayName;
        QString description;
    };

    QList<AssistantInfo> allAssistants = {
        {UOS_AI, "UOS AI", "System's Comprehensive AI Assistant."},
        {UOS_SYSTEM_ASSISTANT, "UOS System Assistant", "Assists you with UOS system-related inquiries"},
        {DEEPIN_SYSTEM_ASSISTANT, "Deepin System Assistant", "Assists you with Deepin system-related inquiries"},
        {PERSONAL_KNOWLEDGE_ASSISTANT, "Personal Knowledge Assistant", "Answers questions based on your personal knowledge base."},
        {AI_WRITING, "AI Writing", "Write Based on Your Topic and Requirements."},
        {AI_TEXT_PROCESSING, "AI Text Processing", "Capable of Handling Text Processing Tasks Such as Summarizing, Proofreading, and Rewriting."},
        {AI_TRANSLATION, "AI Translation", "Your Translation Assistant, Mastering Multiple Languages."}
    };

    for (const auto &info : allAssistants) {
        // Skip system assistant if it's not the correct type for the current environment
        if ((info.type == UOS_SYSTEM_ASSISTANT && Util::isCommunity()) ||
            (info.type == DEEPIN_SYSTEM_ASSISTANT && !Util::isCommunity())) {
            continue;
        }

        if (!existingTypes.contains(info.type)) {
            AssistantProxy proxy;
            proxy.displayName = info.displayName;
            proxy.id = Util::generateAssistantUuid(proxy.displayName);
            proxy.type = info.type;
            proxy.description = info.description;
            appendAssistant(proxy);
        }
    }
}

// 大模型表操作
bool DbWrapper::appendLlm(const LLMServerProxy &llmServerProxy)
{
    qCDebug(logDBS) << "Appending new LLM:" << llmServerProxy.name << "id:" << llmServerProxy.id;
    auto llmTable = LlmTable::create(
                        llmServerProxy.id, llmServerProxy.name
                        , static_cast<int>(llmServerProxy.model), "大型语言模型（LLM）"
                        , QJsonDocument(llmServerProxy.account.toJson()).toJson(QJsonDocument::Compact)
                        , llmServerProxy.toExpandString());
    return llmTable.save();
}

bool DbWrapper::deleteLlm(const QString &lmid)
{
    qCDebug(logDBS) << "Deleting LLM:" << lmid;
    auto llmTable = LlmTable::get(lmid);
    return llmTable.remove();
}

bool DbWrapper::updateLlm(const LLMServerProxy &llmServerProxy)
{
    qCDebug(logDBS) << "Updating LLM:" << llmServerProxy.name << "id:" << llmServerProxy.id;
    auto llmObject = LlmTable::get(llmServerProxy.id);

    llmObject.setName(llmServerProxy.name);
    llmObject.setType(static_cast<int>(llmServerProxy.model));
    llmObject.setAccountProxy(QJsonDocument(llmServerProxy.account.toJson()).toJson(QJsonDocument::Compact));
    llmObject.setExt(llmServerProxy.toExpandString());

    return llmObject.update();
}

LLMServerProxy DbWrapper::queryLlmByLlmid(const QString &lmid)
{
    LLMServerProxy  llmServerProxy;
    auto llmTable = LlmTable::get(lmid);

    llmServerProxy.id = lmid;
    llmServerProxy.model = static_cast<LLMChatModel>(llmTable.type());
    AccountProxy accproxy;
    accproxy.fromJson(QJsonDocument::fromJson(llmTable.accountProxy().toLocal8Bit()).object());
    llmServerProxy.account = accproxy;
    llmServerProxy.fromExpandString(llmTable.ext());

    return llmServerProxy;
}

QList<LLMServerProxy> DbWrapper::queryLlmList(bool all)
{
    QList<LLMServerProxy> llmsvList;
    const auto &llmTableList = LlmTable::getAll();
    for (const auto &llm : llmTableList) {
        LLMChatModel model = static_cast<LLMChatModel>(llm.type());
#ifndef QT_DEBUG
        if (!all && Util::isGPTSeries(model) && !Util::isGPTEnable())
            continue;
#endif

        LLMServerProxy  llmServerProxy;
        llmServerProxy.id = llm.uuid();
        llmServerProxy.model = model;
        llmServerProxy.name = llm.name();
        llmServerProxy.fromExpandString(llm.ext());

        AccountProxy accproxy;
        accproxy.fromJson(QJsonDocument::fromJson(llm.accountProxy().toLocal8Bit()).object());
        llmServerProxy.account = accproxy;

        llmsvList.append(llmServerProxy);
    }
    return llmsvList;
}

// 助手角色表
bool DbWrapper::appendAssistant(const AssistantProxy &assistantProxy)
{
    auto assistantTable = AssistantTable::create(
                        assistantProxy.id, assistantProxy.displayName, assistantProxy.type, assistantProxy.description);
    return assistantTable.save();
}

bool DbWrapper::deleteAssistant(const QString &assistantId)
{
    auto assistantTable = AssistantTable::get(assistantId);
    return assistantTable.remove();
}

bool DbWrapper::updateAssistant(const AssistantProxy &assistantProxy)
{
    auto assistantObject = AssistantTable::get(assistantProxy.id);

    assistantObject.setDisplayName(assistantProxy.displayName);
    assistantObject.setType(assistantProxy.type);
    assistantObject.setDescription(assistantProxy.description);

    return assistantObject.update();
}

bool DbWrapper::updateAssistantId(const AssistantProxy &assistantProxy, const QString &newId)
{
    // 1. 先删除旧记录
    if (!deleteAssistant(assistantProxy.id)) {
        qCWarning(logDBS) << "Failed to delete old assistant record:" << assistantProxy.id;
        return false;
    }

    // 2. 创建新记录
    AssistantProxy newAssistant = assistantProxy;
    newAssistant.id = newId;
    
    if (!appendAssistant(newAssistant)) {
        qCWarning(logDBS) << "Failed to create new assistant record:" << newId;
        // 如果创建失败，恢复旧记录
        appendAssistant(assistantProxy);
        return false;
    }

    return true;
}

AssistantProxy DbWrapper::queryAssistantByid(const QString &assistantId)
{
    AssistantProxy  assistantProxy;
    auto assistantTable = AssistantTable::get(assistantId);
#ifndef ENABLE_ASSISTANT
    if (!m_allowedTypes.contains(static_cast<AssistantType>(assistantTable.type())))
        return assistantProxy;
#endif
#ifndef ENABLE_PERSONAL_KNOWLEDGE_ASSISTANT
    if (assistantTable.type() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT)
        return assistantProxy;
#endif
    assistantProxy.id = assistantTable.id();
    assistantProxy.displayName = assistantTable.displayName();
    assistantProxy.type = static_cast<AssistantType>(assistantTable.type());
    assistantProxy.description = assistantTable.description();

    return assistantProxy;
}

QList<AssistantProxy> DbWrapper::queryAssistantList(bool all)
{
    QList<AssistantProxy> assistantList;
    const auto &assistantTableList = AssistantTable::getAll();

    for (const auto &assistant : assistantTableList) {
#ifndef ENABLE_ASSISTANT
        if (!m_allowedTypes.contains(static_cast<AssistantType>(assistant.type())))
            continue;
#endif
#ifndef ENABLE_PERSONAL_KNOWLEDGE_ASSISTANT
        if (assistant.type() == AssistantType::PERSONAL_KNOWLEDGE_ASSISTANT)
            continue;
#endif
        AssistantProxy  assistantProxy;
        assistantProxy.id = assistant.id();
        assistantProxy.displayName = assistant.displayName();
        assistantProxy.type = static_cast<AssistantType>(assistant.type());
        assistantProxy.description = assistant.description();

        assistantList.append(assistantProxy);
    }

    // Sort the list by assistant type
    std::sort(assistantList.begin(), assistantList.end(),
        [](const AssistantProxy &a, const AssistantProxy &b) {
            return static_cast<int>(a.type) < static_cast<int>(b.type);
        });

    return assistantList;
}

// 助手当前模型
bool DbWrapper::appendAssistantLlm(const QString &assistantId, const QString &llmId)
{
    auto assistantLlm = CurLlmTable::create(assistantId, llmId);
    return assistantLlm.save();
}

bool DbWrapper::deleteAssistantLlm(const QString &assistantId)
{
    auto assistantLlm = CurLlmTable::get(assistantId);
    return assistantLlm.remove();
}

bool DbWrapper::updateAssistantLlm(const QString &assistantId, const QString &llmId)
{
    auto assistantLlm = CurLlmTable::get(assistantId);
    if (assistantLlm.assistantTd().isEmpty()) {
        assistantLlm.setAssistantId(assistantId);
        assistantLlm.setLlmId(llmId);
        return assistantLlm.save();
    } else {
        assistantLlm.setLlmId(llmId);
        return assistantLlm.update();
    }
}

QString DbWrapper::queryLlmIdByAssistantId(const QString &assistantId)
{
    const auto &assistantLlmTableList = CurLlmTable::getAll();
    for (const auto &assistantLlm : assistantLlmTableList) {
        if (assistantLlm.assistantTd() == assistantId)
            return assistantLlm.llmId();
    }

    return "";
}

// App表的操作
bool DbWrapper::appendApp(const AppDbusPathObject &appObject)
{
    auto appTable = AppTable::create(
                        appObject.appId, appObject.path, "", appObject.curLLMId
                        , QJsonDocument(QJsonObject::fromVariantMap(appObject.cmdPrompts)).toJson()
                        , "");
    return appTable.save();
}

bool DbWrapper::deleteApp(const QString &appid)
{
    auto appTable = AppTable::get(appid);
    return appTable.remove();
}

bool DbWrapper::updateApp(const AppDbusPathObject &appObject)
{
    auto appTable = AppTable::get(appObject.appId);
    appTable.setDesc(appObject.path);
    appTable.setLlmid(appObject.curLLMId);
    QString cmdPromptsStr = QJsonDocument(QJsonObject::fromVariantMap(appObject.cmdPrompts)).toJson();
    appTable.setCmd(cmdPromptsStr);
    return appTable.update();
}

bool DbWrapper::updateAppCurllmId(const QString &appid, const QString &llmId)
{
    auto appTable = AppTable::get(appid);
    if (appTable.uuid().isEmpty()) {
        //如果没有app, 新建app对象再执行更新
        appTable.setUuid(appid);
        appTable.setLlmid(llmId);
        return appTable.save();
    } else {
        appTable.setLlmid(llmId);
        return appTable.update();
    }
}

bool DbWrapper::updateAppCurAssistantId(const QString &appid, const QString &assistantId)
{
    auto appTable = AppTable::get(appid);
    if (appTable.uuid().isEmpty()) {
        //如果没有app, 新建app对象再执行更新
        appTable.setUuid(appid);
        appTable.setAssistantid(assistantId);
        return appTable.save();
    } else {
        appTable.setAssistantid(assistantId);
        return appTable.update();
    }
}

AppDbusPathObject DbWrapper::queryAppByAppId(const QString &appid)
{
    AppDbusPathObject appDbusObject;
    auto appTable = AppTable::get(appid);

    appDbusObject.appId = appid;
    appDbusObject.path = LLMUtils::adjustDbusPath(appDbusObject.appId);
    appDbusObject.curLLMId = appTable.llmid();

    auto cmdJsonObject = QJsonDocument::fromJson(appTable.cmd().toLocal8Bit()).object();
    appDbusObject.cmdPrompts = cmdJsonObject.toVariantMap();

    return appDbusObject;
}

QString DbWrapper::queryCurLlmIdByAppId(const QString &appid)
{
    auto appTable = AppTable::get(appid);
    return appTable.llmid();
}

QString DbWrapper::queryCurAssistantIdByAppId(const QString &appid)
{
    auto appTable = AppTable::get(appid);
    return appTable.assistantid();
}

QMap<QString, AppDbusPathObject> DbWrapper::queryAppList()
{
    QMap<QString, AppDbusPathObject> appDbugList;
    const auto &appTableList = AppTable::getAll();

    for (const auto &appTable : appTableList) {
        AppDbusPathObject appDbusObject;

        appDbusObject.appId = appTable.uuid();
        appDbusObject.path = LLMUtils::adjustDbusPath(appDbusObject.appId);
        appDbusObject.curLLMId = appTable.llmid();

        auto cmdJsonObject = QJsonDocument::fromJson(appTable.cmd().toLocal8Bit()).object();
        appDbusObject.cmdPrompts = cmdJsonObject.toVariantMap();

        appDbugList[appDbusObject.appId] = appDbusObject;
    }
    return appDbugList;
}

// Config 配置表
QString DbWrapper::getVersion()
{
    auto version = ConfigTable::get(ConfigTable::CopilotVersion);
    return version.value();
}

bool DbWrapper::updateVersion(const QString &newVersion)
{
    auto version = ConfigTable::get(ConfigTable::CopilotVersion);
    auto strVer = version.value();
    if (version.id() <= 0) {
        version.setName("Version");
        version.setDesc("");
        version.setType(ConfigTable::CopilotVersion);
        version.setValue(newVersion);
        return version.save();
    } else if (strVer != newVersion) {
        version.setValue(newVersion);
        return version.update();
    }
    return true;
}

QString DbWrapper::getCopilotTheme()
{
    auto theme = ConfigTable::get(ConfigTable::CopilotTheme);
    return theme.value();
}

bool DbWrapper::updateCopilotTheme(const QString &newTheme)
{
    auto theme = ConfigTable::get(ConfigTable::CopilotTheme);
    auto strTheme = theme.value();
    if (theme.id() <= 0) {
        theme.setName("Theme");
        theme.setDesc("");
        theme.setType(ConfigTable::CopilotTheme);
        theme.setValue(newTheme);
        return theme.save();
    } else if (strTheme != newTheme) {
        theme.setValue(newTheme);
        return theme.update();
    }
    return true;
}

bool DbWrapper::getAICopilotIsOpen()
{
    auto swit = ConfigTable::get(ConfigTable::CopilotSwitch);
    return (swit.value().toInt() == 1 ? true : false);
}

bool DbWrapper::updateAICopilot(bool isOpen)
{
    auto swit = ConfigTable::get(ConfigTable::CopilotSwitch);
    if (swit.id() <= 0) {
        swit.setName("CopilotSwitch");
        swit.setDesc("");
        swit.setType(ConfigTable::CopilotSwitch);
        swit.setValue(isOpen ? "1" : "0");
        return swit.save();
    } else {
        swit.setValue(isOpen ? "1" : "0");
        return swit.update();
    }
}

bool DbWrapper::getThirdPartyMcpAgreement()
{
    auto swit = ConfigTable::get(ConfigTable::CopilotThirdPartyMcp);
    return (swit.value().toInt() == 1 ? true : false);
}

bool DbWrapper::updateThirdPartyMcpAgreement(bool isAgree)
{
    auto swit = ConfigTable::get(ConfigTable::CopilotThirdPartyMcp);
    if (swit.id() <= 0) {
        swit.setName("ThirdPartyMcp");
        swit.setDesc("");
        swit.setType(ConfigTable::CopilotThirdPartyMcp);
        swit.setValue(isAgree ? "1" : "0");
        return swit.save();
    } else {
        swit.setValue(isAgree ? "1" : "0");
        return swit.update();
    }
}

int DbWrapper::getUserExpState()
{
    auto swit = ConfigTable::get(ConfigTable::CopilotUserExp);
    return swit.value().toInt();
}

bool DbWrapper::updateUserExpState(int state)
{
    auto swit = ConfigTable::get(ConfigTable::CopilotUserExp);
    if (swit.id() <= 0) {
        swit.setName("UserExp");
        swit.setDesc("");
        swit.setType(ConfigTable::CopilotUserExp);
        swit.setValue(QString::number(state));
        return swit.save();
    } else {
        swit.setValue(QString::number(state));
        return swit.update();
    }
}

bool DbWrapper::getLocalSpeech()
{
    auto swit = ConfigTable::get(ConfigTable::CopilotLocalSpeech);
    return swit.value().toInt() == 1;
}

bool DbWrapper::updateLocalSpeech(bool isOpen)
{
    auto swit = ConfigTable::get(ConfigTable::CopilotLocalSpeech);
    if (swit.id() <= 0) {
        swit.setName("LocalSpeech");
        swit.setDesc("");
        swit.setType(ConfigTable::CopilotLocalSpeech);
        swit.setValue(isOpen ? "1" : "0");
        return swit.save();
    } else {
        swit.setValue(isOpen ? "1" : "0");
        return swit.update();
    }
}

int DbWrapper::getDisplayMode()
{
    auto mode = ConfigTable::get(ConfigTable::WindowDisplayMode);
    return mode.value().toInt();
}
bool DbWrapper::updateDisplayMode(int mode)
{
    auto displayMode = ConfigTable::get(ConfigTable::WindowDisplayMode);
    auto strMode = displayMode.value();
    if (displayMode.id() <= 0) {
        displayMode.setName("DisplayMode");
        displayMode.setDesc("");
        displayMode.setType(ConfigTable::WindowDisplayMode);
        displayMode.setValue(QString::number(mode));
        return displayMode.save();
    } else if (strMode != QString::number(mode)) {
        displayMode.setValue(QString::number(mode));
        return displayMode.update();
    }
    return true;
}

QString DbWrapper::getWindowSize()
{
    auto windowSize = ConfigTable::get(ConfigTable::WindowSize);
    return windowSize.value();
}
bool DbWrapper::updateWindowSize(QString size)
{
    auto windowSize = ConfigTable::get(ConfigTable::WindowSize);
    auto strMode = windowSize.value();
    if (windowSize.id() <= 0) {
        windowSize.setName("WindowSize");
        windowSize.setDesc("");
        windowSize.setType(ConfigTable::WindowSize);
        windowSize.setValue(size);
        return windowSize.save();
    } else if (strMode != size) {
        windowSize.setValue(size);
        return windowSize.update();
    }
    return true;
}

QString DbWrapper::getGuideKey()
{
    auto guide = ConfigTable::get(ConfigTable::CopilotGuide);
    return guide.value();
}

bool DbWrapper::updateGuideKey(const QString &key)
{
    auto guide = ConfigTable::get(ConfigTable::CopilotGuide);
    auto strMode = guide.value();
    if (guide.id() <= 0) {
        guide.setName("CopilotGuide");
        guide.setDesc("");
        guide.setType(ConfigTable::CopilotGuide);
        guide.setValue(key);
        return guide.save();
    } else if (strMode != key) {
        guide.setValue(key);
        return guide.update();
    }

    return true;
}

int DbWrapper::getUpdatePromptBits(int startBit, int length)
{
    auto guide = ConfigTable::get(ConfigTable::CopilotUpdatePrompt);
    QString value = guide.value();
    
    if (value.isEmpty()) {
        return 0;
    }

    bool ok;
    int binaryData = value.toInt(&ok, 2);
    if (!ok) {
        return 0;
    }
    
    int mask = (1 << length) - 1;
    int result = (binaryData >> startBit) & mask;
    
    return result;
}

bool DbWrapper::updateUpdatePromptBits(int startBit, int length, int value)
{
    // 0-1位 快捷键更新弹窗
    // 2位 智能体更新弹窗
    // 3位 MCP更新弹窗
    // 4位 隐私对话更新弹窗
    auto guide = ConfigTable::get(ConfigTable::CopilotUpdatePrompt);
    QString currentValue = guide.value();
    
    bool ok = false;
    int binaryData = currentValue.isEmpty() ? 0 : currentValue.toInt(&ok, 2);
    if (!ok) {
        binaryData = 0;
    }
    int mask = (1 << length) - 1;
    int clearMask = ~(mask << startBit);

    binaryData = (binaryData & clearMask) | ((value & mask) << startBit);

    QString newValue = QString::number(binaryData, 2);
    if (guide.id() <= 0) {
        guide.setName("CopilotUpdatePrompt");
        guide.setDesc("");
        guide.setType(ConfigTable::CopilotUpdatePrompt);
        guide.setValue(newValue);
        return guide.save();
    } else {
        guide.setValue(newValue);
        return guide.update();
    }
}

int DbWrapper::getUpdatePromptBits(UpdatePromptBitType type)
{
    return getUpdatePromptBits(getStartBit(type), getLength(type));
}

bool DbWrapper::updateUpdatePromptBits(UpdatePromptBitType type, int value)
{
    return updateUpdatePromptBits(getStartBit(type), getLength(type), value);
}
