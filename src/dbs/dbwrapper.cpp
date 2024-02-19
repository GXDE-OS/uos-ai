#include "dbwrapper.h"
#include "daoclient.h"
#include "tables/apptable.h"
#include "tables/llmtable.h"
#include "tables/configtable.h"
#include "llmutils.h"

#include <QDebug>
#include <QJsonDocument>

static QString gVersion = "Version_1_0";

QMap<CopilotDbType, QString> DbWrapper::m_qMapDatabases = {
    {CopilotDbType::COPILOT_BASIC,       QString("basic")},
    {CopilotDbType::COPILOT_CHATINFO,    QString("chatinfo")}
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

    DaoResultListPtr result;
    QString message;

    if (!m_daoClient->execSync(strAppInfo, result, message,  m_qMapDatabases[COPILOT_BASIC])) {
        qInfo() << message;
    }
    if (!m_daoClient->execSync(strLlmInfo, result, message,  m_qMapDatabases[COPILOT_BASIC])) {
        qInfo() << message;
    }
    if (!m_daoClient->execSync(strConfigInfo, result, message,  m_qMapDatabases[COPILOT_BASIC])) {
        qInfo() << message;
    }

    const auto &version = getVersion();
    if (version.isEmpty()) {
        updateVersion(gVersion);
        updateCopilotTheme("Light");
        updateAICopilot(false);
        updateUserExpState(0);
    }

    return 0;
}


// 大模型表操作

bool DbWrapper::appendLlm(const LLMServerProxy &llmServerProxy)
{
    auto llmTable = LlmTable::create(
                        llmServerProxy.id, llmServerProxy.name
                        , static_cast<int>(llmServerProxy.model), "大型语言模型（LLM）"
                        , QJsonDocument(llmServerProxy.account.toJson()).toJson(QJsonDocument::Compact)
                        , llmServerProxy.toExpandString());
    return llmTable.save();
}

bool DbWrapper::deleteLlm(const QString &lmid)
{
    auto llmTable = LlmTable::get(lmid);
    return llmTable.remove();
}

bool DbWrapper::updateLlm(const LLMServerProxy &llmServerProxy)
{
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
        if (QLocale::Chinese == QLocale::system().language() && QLocale::SimplifiedChineseScript == QLocale::system().script() && model < LLMChatModel::SPARKDESK && !all)
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
