#include "toolhandler.h"
#include "oscontrol/deepinabilitymanager.h"
#include "oscallcontext.h"
#include "global_define.h"

#include <QLoggingCategory>
#include <QJsonArray>
#include <QJsonDocument>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {

ToolCallResult ToolHandler::callTool(const QString &toolName, const QJsonObject &params)
{
    // 系统控制工具
    if (toolName == "switchWifi" || toolName == "openBluetooth" ||
        toolName == "switchNoDisturbMode" || toolName == "switchEyesProtection" ||
        toolName == "displayBrightness" || toolName == "volumeAdjustment" ||
        toolName == "switchSystemTheme" || toolName == "switchPerformanceMode" ||
        toolName == "switchWallpaper" || toolName == "switchDisplayMode" ||
        toolName == "switchScreen" || toolName == "shutdownFront" ||
        toolName == "launchApplication" || toolName == "systemTotalMemory" ||
        toolName == "systemFontSize" || toolName == "getBluetoothDevices" ||
        toolName == "getSystemFontSize") {
        return handleSystemTool(toolName, params);
    }

    // 文件操作工具
    if (toolName == "openFile" || toolName == "copyFile" ||
        toolName == "moveFile" || toolName == "createFolder" ||
        toolName == "renameFile" || toolName == "batchRename" ||
        toolName == "readFile" || toolName == "getFileMetadata") {
        return handleFileTool(toolName, params);
    }

    // 媒体控制工具
    if (toolName == "musicControl") {
        return handleMediaTool(toolName, params);
    }

    // 通信工具
    if (toolName == "createSchedule" || toolName == "sendMail") {
        return handleCommunicationTool(toolName, params);
    }

    // 应用商店工具
    if (toolName == "searchAndInstallApp") {
        return handleStoreTool(toolName, params);
    }

    qCWarning(logAgent) << "Unknown tool:" << toolName;
    ToolCallResult result(-1, QString("Unknown tool: %1").arg(toolName));
    result.toolName = toolName;
    return result;
}

ToolCallResult ToolHandler::handleSystemTool(const QString &toolName, const QJsonObject &params)
{
    OSCallContext ctx;
    ToolCallResult result;
    result.toolName = toolName;

    if (toolName == "switchWifi") {
        bool on = params.value("switch").toBool();
        ctx = UosAbility()->switchWifi(on);
        result.cardType = CardType::SwitchCard;
        result.cardData["switch"] = on;
        result.cardData["title"] = "Wireless Network";
        result.cardData["icon"] = "wifi";
    }
    else if (toolName == "openBluetooth") {
        bool on = params.value("switch").toBool();
        ctx = UosAbility()->doBluetoothConfig(on);
        result.cardType = CardType::SwitchCard;
        result.cardData["switch"] = on;
        result.cardData["title"] = "Bluetooth";
        result.cardData["icon"] = "bluetooth";
    }
    else if (toolName == "switchNoDisturbMode") {
        bool state = params.value("switch").toBool();
        ctx = UosAbility()->doNoDisturb(state);
        result.cardType = CardType::SwitchCard;
        result.cardData["switch"] = state;
        result.cardData["title"] = "DND Mode";
        result.cardData["icon"] = "no_disturb";
    }
    else if (toolName == "switchEyesProtection") {
        bool on = params.value("switch").toBool();
        ctx = UosAbility()->doDiplayEyesProtection(on);
        result.cardType = CardType::SwitchCard;
        result.cardData["switch"] = on;
        result.cardData["title"] = "Eye Comfort";
        result.cardData["icon"] = "eyes_protection";
    }
    else if (toolName == "displayBrightness") {
        int percent = params.value("percent").toInt();
        int adjustment = params.value("adjustment").toInt(0);
        ctx = UosAbility()->doDiplayBrightness(percent, adjustment);
        result.cardType = CardType::SliderCard;
        result.cardData["percent"] = ctx.result.contains("brightness") ? ctx.result.value("brightness").toInt() : percent;
        result.cardData["title"] = "Brightness";
        result.cardData["icon"] = "brightness";
        result.cardData["min"] = 0;
        result.cardData["max"] = 100;
    }
    else if (toolName == "volumeAdjustment") {
        ctx = UosAbility()->volumeAdjustment(params);
        result.cardType = CardType::SliderCard;
        result.cardData["percent"] = ctx.result.contains("volume") ? ctx.result.value("volume").toInt() : 0;
        result.cardData["title"] = "Volume";
        result.cardData["icon"] = "volume";
        result.cardData["min"] = 0;
        result.cardData["max"] = 100;
    }
    else if (toolName == "switchSystemTheme") {
        QString theme = params.value("theme").toString();
        int themeValue = 0;
        if (theme == "Dark") {
            themeValue = 1;
        } else if (theme == "Auto") {
            themeValue = 2;
        }
        ctx = UosAbility()->doSystemThemeSwitch(themeValue);
        result.cardType = CardType::None; // 主题切换直接输出结果
    }
    else if (toolName == "switchPerformanceMode") {
        QString mode = params.value("mode").toString();
        bool isOpen = params.value("switch").toBool();
        ctx = UosAbility()->doPerformanceModeSwitch(mode, isOpen);
        result.cardType = CardType::None; // 性能模式切换直接输出结果
    }
    else if (toolName == "switchWallpaper") {
        ctx = UosAbility()->doWallpaperSwitch();
        result.cardType = CardType::None; // 壁纸切换直接输出结果
    }
    else if (toolName == "switchDisplayMode") {
        QString modestr = params.value("mode").toString();
        int modeValue = -1;
        if (modestr == "Copy")
            modeValue = 1;
        else if (modestr == "Extend")
            modeValue = 2;

        if (modeValue == -1) {
            qCWarning(logAgent) << "switchDisplayMode invalid mode:" << modestr;
            result.errorCode = 0;
            result.message = QString("Invalid display mode: %1. Supported modes are Copy and Extend.").arg(modestr);
            return result;
        }
        ctx = UosAbility()->doDisplayModeSwitch(modeValue);
        result.cardType = CardType::None;
    }
    else if (toolName == "switchScreen") {
        ctx = UosAbility()->switchScreen();
        result.cardType = CardType::None; // 屏幕切换直接输出结果
    }
    else if (toolName == "shutdownFront") {
        ctx = UosAbility()->openShutdownFront();
        result.cardType = CardType::None; // 关机界面直接输出结果
    }
    else if (toolName == "launchApplication") {
        QString appid = params.value("appid").toString();
        bool on = params.value("switch").toBool();
        ctx = UosAbility()->doAppLaunch(appid, on);
        result.cardType = CardType::None;
    }
    else if (toolName == "systemTotalMemory") {
        ctx = UosAbility()->getSystemMemory();
        result.cardType = CardType::None; // 系统内存信息直接输出结果
    }
    else if (toolName == "systemFontSize") {
        float size = params.value("size").toVariant().toFloat();
        ctx = UosAbility()->doSystemFontSize(size);
        result.cardType = CardType::SliderCard;
        result.cardData["size"] = ctx.result.contains("fontSize") ? ctx.result.value("fontSize").toDouble() : static_cast<double>(size);
        result.cardData["title"] = "Font Size";
        result.cardData["icon"] = "font_size";
        result.cardData["min"] = 11.0;
        result.cardData["max"] = 20.0;
        result.cardData["step"] = 1;
    }
    else if (toolName == "getBluetoothDevices") {
        ctx = UosAbility()->doGetBluetoothDevices();
        result.cardType = CardType::None; // 蓝牙设备列表直接输出结果
    }
    else if (toolName == "getSystemFontSize") {
        ctx = UosAbility()->getSystemFontSize();
        result.cardType = CardType::None; 
    }

    // 处理返回值
    if (ctx.error == OSCallContext::NonError) {
        result.errorCode = 0;
        result.message = ctx.output.isEmpty() ? "操作成功" : ctx.output;
    } else {
        result.errorCode = static_cast<int>(ctx.error);
        result.message = ctx.errorInfo.isEmpty() ? "操作失败" : ctx.errorInfo;
        if (!ctx.output.isEmpty()) {
            result.message += " - " + ctx.output;
        }
    }

    return result;
}

ToolCallResult ToolHandler::handleFileTool(const QString &toolName, const QJsonObject &params)
{
    OSCallContext ctx;
    ToolCallResult result;
    result.toolName = toolName;
    result.cardType = CardType::None; // 文件操作直接输出结果

    if (toolName == "openFile") {
        QString filePath = params.value("filePath").toString();
        ctx = UosAbility()->doOpenFile(filePath);
    }
    else if (toolName == "copyFile") {
        QString sourcePath = params.value("sourcePath").toString();
        QString destinationPath = params.value("destinationPath").toString();
        ctx = UosAbility()->doCopyFile(sourcePath, destinationPath);
    }
    else if (toolName == "moveFile") {
        QString sourcePath = params.value("sourcePath").toString();
        QString destinationPath = params.value("destinationPath").toString();
        ctx = UosAbility()->doMoveFile(sourcePath, destinationPath);
    }
    else if (toolName == "createFolder") {
        QString folderPath = params.value("folderPath").toString();
        ctx = UosAbility()->doCreateFolder(folderPath);
    }
    else if (toolName == "renameFile") {
        QString oldPath = params.value("oldPath").toString();
        QString newName = params.value("newName").toString();
        ctx = UosAbility()->doRenameFile(oldPath, newName);
    }
    else if (toolName == "batchRename") {
        QString folderPath = params.value("folderPath").toString();
        QString newName = params.value("newName").toString();
        QString pattern = params.value("pattern").toString();
        ctx = UosAbility()->doBatchRename(folderPath, newName, pattern);
    }
    else if (toolName == "readFile") {
        QString filePath = params.value("filePath").toString();
        ctx = UosAbility()->doReadFile(filePath);
    }
    else if (toolName == "getFileMetadata") {
        QJsonArray fileList = params.value("fileList").toArray();
        QStringList filePaths;
        for (const QJsonValue &value : fileList) {
            filePaths.append(value.toString());
        }
        ctx = UosAbility()->doGetFileMetadata(filePaths);
    }

    // 处理返回值
    if (ctx.error == OSCallContext::NonError) {
        result.errorCode = 0;
        result.message = ctx.output.isEmpty() ? "操作成功" : ctx.output;
    } else {
        result.errorCode = static_cast<int>(ctx.error);
        result.message = ctx.errorInfo.isEmpty() ? "操作失败" : ctx.errorInfo;
        if (!ctx.output.isEmpty()) {
            result.message += " - " + ctx.output;
        }
    }

    return result;
}

ToolCallResult ToolHandler::handleMediaTool(const QString &toolName, const QJsonObject &params)
{
    OSCallContext ctx;
    ToolCallResult result;
    result.toolName = toolName;
    result.cardType = CardType::None; // 媒体控制直接输出结果

    if (toolName == "musicControl") {
        QString action = params.value("action").toString();
        // 根据action类型调用不同的函数
        if (action == "Seek") {
            int offset = params.value("offset").toInt();
            ctx = UosAbility()->doSeek(offset);
        } else {
            ctx = UosAbility()->doStateControl(action);
        }
    }

    result.errorCode = static_cast<int>(ctx.error);
    result.message = ctx.output;
    return result;
}

ToolCallResult ToolHandler::handleCommunicationTool(const QString &toolName, const QJsonObject &params)
{
    OSCallContext ctx;
    ToolCallResult result;
    result.toolName = toolName;

    if (toolName == "createSchedule") {
        QString subject = params.value("subject").toString();
        QString startTime = params.value("startTime").toString();
        QString endTime = params.value("endTime").toString();
        ctx = UosAbility()->doCreateSchedule(subject, startTime, endTime);
        result.cardType = CardType::ScheduleCard;
        result.cardData["subject"] = subject;
        result.cardData["startTime"] = startTime;
        result.cardData["endTime"] = endTime;
        result.cardData["title"] = "日程";
        result.cardData["icon"] = "schedule";
    } else if (toolName == "sendMail") {
        QString to = params.value("to").toString();
        QString subject = params.value("subject").toString();
        QString content = params.value("content").toString();
        QString cc = params.value("cc").toString();
        QString bcc = params.value("bcc").toString();
        
        QStringList toList = to.split(',', PARAM_SKIP_EMPTY);
        QStringList ccList = cc.split(',', PARAM_SKIP_EMPTY);
        QStringList bccList = bcc.split(',', PARAM_SKIP_EMPTY);
        
        ctx = UosAbility()->doSendMail(subject, content, toList, ccList, bccList);
        result.cardType = CardType::None; 
    }

    // 处理返回值
    if (ctx.error == OSCallContext::NonError) {
        result.errorCode = 0;
        result.message = ctx.output.isEmpty() ? "操作成功" : ctx.output;
    } else {
        result.errorCode = static_cast<int>(ctx.error);
        result.message = ctx.errorInfo.isEmpty() ? "操作失败" : ctx.errorInfo;
        if (!ctx.output.isEmpty()) {
            result.message += " - " + ctx.output;
        }
    }

    return result;
}

ToolCallResult ToolHandler::handleStoreTool(const QString &toolName, const QJsonObject &params)
{
    OSCallContext ctx;
    ToolCallResult result;
    result.toolName = toolName;

    if (toolName == "searchAndInstallApp") {
        QString keyword = params.value("keyword").toString();
        int page = params.value("page").toInt(1);
        int maxResults = params.value("maxResults").toInt(3);
        ctx = UosAbility()->doSearchApp(keyword, page, maxResults);
        result.cardType = CardType::AppStoreCard;
        result.cardData["title"] = "应用商店";
        result.cardData["icon"] = "appstore";

        // 解析搜索结果
        if (ctx.error == OSCallContext::NonError && !ctx.output.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(ctx.output.toUtf8());
            if (doc.isObject()) {
                QJsonObject responseObj = doc.object();
                QJsonArray resultsArray = responseObj["results"].toArray();

                QJsonArray appsArray;
                for (int i = 0; i < resultsArray.size() && i < maxResults; i++) {
                    QJsonObject appObj = resultsArray[i].toObject();
                    QJsonObject appInfo;
                    appInfo["name"] = appObj["appName"].toString();
                    appInfo["package"] = appObj["packageName"].toString();
                    appInfo["desc"] = appObj["description"].toString();
                    appInfo["downloads"] = appObj["downloadCount"].toInt();
                    appInfo["rating"] = appObj["score"].toDouble();
                    appInfo["icon"] = appObj["icon"].toString();
                    appsArray.append(appInfo);
                }
                result.extraData["apps"] = appsArray;
                result.cardData["apps"] = appsArray;
            }
        }
    }

    // 处理返回值
    if (ctx.error == OSCallContext::NonError) {
        result.errorCode = 0;
        result.message = ctx.output.isEmpty() ? "操作成功" : ctx.output;
    } else {
        result.errorCode = static_cast<int>(ctx.error);
        result.message = ctx.errorInfo.isEmpty() ? "操作失败" : ctx.errorInfo;
        if (!ctx.output.isEmpty()) {
            result.message += " - " + ctx.output;
        }
    }

    return result;
}

} // namespace uos_ai
