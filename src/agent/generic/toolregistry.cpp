#include "toolregistry.h"

namespace uos_ai {

QList<ModelTool> ToolRegistry::getAllTools()
{
    QList<ModelTool> tools;

    registerSystemTools(tools);
    registerFileTools(tools);
    registerMediaTools(tools);
    registerCommunicationTools(tools);
    registerStoreTools(tools);

    return tools;
}

void ToolRegistry::registerSystemTools(QList<ModelTool> &tools)
{
    // Tool: switchWifi - 连接打开查看无线网络或wifi
    {
        ModelTool tool;
        tool.name = "switchWifi";
        tool.description = "连接打开查看无线网络或wifi。";

        ModelToolProperty prop;
        prop.name = "switch";
        prop.type = "boolean";
        prop.description = "true开启,false关闭";
        tool.properties.append(prop);

        tool.required.append("switch");
        tools.append(tool);
    }

    // Tool: openBluetooth - 连接打开查看无线或蓝牙设备
    {
        ModelTool tool;
        tool.name = "openBluetooth";
        tool.description = "连接打开查看无线或蓝牙设备，如无线鼠标，蓝牙耳机，蓝牙音响。";

        ModelToolProperty prop;
        prop.name = "switch";
        prop.type = "boolean";
        prop.description = "true开启,false关闭";
        tool.properties.append(prop);

        tool.required.append("switch");
        tools.append(tool);
    }

    // Tool: switchNoDisturbMode - 设置勿扰模式开关
    {
        ModelTool tool;
        tool.name = "switchNoDisturbMode";
        tool.description = "设置勿扰模式开关";

        ModelToolProperty prop;
        prop.name = "switch";
        prop.type = "boolean";
        prop.description = "true开启,false关闭";
        tool.properties.append(prop);

        tool.required.append("switch");
        tools.append(tool);
    }

    // Tool: switchEyesProtection - 调整屏幕为护眼模式或阅读模式
    {
        ModelTool tool;
        tool.name = "switchEyesProtection";
        tool.description = "调整屏幕为护眼模式或阅读模式";

        ModelToolProperty prop;
        prop.name = "switch";
        prop.type = "boolean";
        prop.description = "true开启,false关闭";
        tool.properties.append(prop);

        tool.required.append("switch");
        tools.append(tool);
    }

    // Tool: displayBrightness - 屏幕亮度
    {
        ModelTool tool;
        tool.name = "displayBrightness";
        tool.description = "屏幕亮度调整；亮度大一点、亮度小一点属于模糊调整。";

        ModelToolProperty percentProp;
        percentProp.name = "percent";
        percentProp.type = "integer";
        percentProp.description = "准确亮度值调整，亮度大小，百分比,值范围0到100。";
        tool.properties.append(percentProp);

        ModelToolProperty adjustmentProp;
        adjustmentProp.name = "adjustment";
        adjustmentProp.type = "integer";
        adjustmentProp.description = "调整方式：0=绝对值调整(使用percent参数)，1=增加亮度，2=降低亮度。模糊调整时使用1或2。";
        tool.properties.append(adjustmentProp);

        tools.append(tool);
    }

    // Tool: volumeAdjustment - 音量调整
    {
        ModelTool tool;
        tool.name = "volumeAdjustment";
        tool.description = "音量调整；声音大一点、声音小一点属于模糊调整。";

        ModelToolProperty volumeProp;
        volumeProp.name = "volume";
        volumeProp.type = "integer";
        volumeProp.description = "准确音量值调整，音量大小，百分比,值范围0到100。";
        tool.properties.append(volumeProp);

        ModelToolProperty approximateProp;
        approximateProp.name = "approximate";
        approximateProp.type = "string";
        approximateProp.description = "模糊调整；增大音量、声音大一点为Add；减小音量、声音小一点为Reduce";
        approximateProp.enums.append("Add");
        approximateProp.enums.append("Reduce");
        tool.properties.append(approximateProp);

        ModelToolProperty muteProp;
        muteProp.name = "mute";
        muteProp.type = "boolean";
        muteProp.description = "true静音、关闭声音，false打开声音、开启声音。";
        tool.properties.append(muteProp);

        tools.append(tool);
    }

    // Tool: switchSystemTheme - 切换或设置系统主题
    {
        ModelTool tool;
        tool.name = "switchSystemTheme";
        tool.description = "切换或设置系统主题，如：浅色，暗色";

        ModelToolProperty prop;
        prop.name = "theme";
        prop.type = "string";
        prop.description = "Light浅色,Dark暗色,Auto跟随系统";
        prop.enums.append("Light");
        prop.enums.append("Dark");
        prop.enums.append("Auto");
        tool.properties.append(prop);

        tool.required.append("theme");
        tools.append(tool);
    }

    // Tool: switchPerformanceMode - 打开、切换和关闭电源的性能模式
    {
        ModelTool tool;
        tool.name = "switchPerformanceMode";
        tool.description = "打开、切换和关闭电源的性能模式，例如：高性能模式、平衡模式、电源节能模式。只对描述的模式进行开启关闭操作。";

        ModelToolProperty modeProp;
        modeProp.name = "mode";
        modeProp.type = "string";
        modeProp.description = "性能模式（值：performance、balance、powersave）";
        tool.properties.append(modeProp);

        ModelToolProperty switchProp;
        switchProp.name = "switch";
        switchProp.type = "boolean";
        switchProp.description = "true打开,false关闭";
        tool.properties.append(switchProp);

        tools.append(tool);
    }

    // Tool: switchWallpaper - 切换壁纸
    {
        ModelTool tool;
        tool.name = "switchWallpaper";
        tool.description = "切换壁纸";
        tools.append(tool);
    }

    // Tool: switchDisplayMode - 切换屏幕模式
    {
        ModelTool tool;
        tool.name = "switchDisplayMode";
        tool.description = "切换屏幕显示模式：复制/镜像模式（Copy）或扩展模式（Extend）";

        ModelToolProperty prop;
        prop.name = "mode";
        prop.type = "string";
        prop.description = "屏幕模式，取值必须为 'Copy'（复制/镜像屏幕）或 'Extend'（扩展屏幕）";
        prop.enums.append("Copy");
        prop.enums.append("Extend");
        tool.properties.append(prop);

        tool.required.append("mode");
        tools.append(tool);
    }

    // Tool: switchScreen - 切换屏幕
    {
        ModelTool tool;
        tool.name = "switchScreen";
        tool.description = "切换屏幕";
        tools.append(tool);
    }

    // Tool: shutdownFront - 关机等系统操作
    {
        ModelTool tool;
        tool.name = "shutdownFront";
        tool.description = "关机等系统操作";
        tools.append(tool);
    }

    // Tool: launchApplication - 启动应用
    {
        ModelTool tool;
        tool.name = "launchApplication";
        tool.description = "打开、启动、运行某个应用，关闭、退出、结束某个应用";

        ModelToolProperty appidProp;
        appidProp.name = "appid";
        appidProp.type = "string";
        appidProp.description = "打开、启动、运行某个应用，关闭、退出、结束某个应用；其中记事本是指deepinVoiceNote";
        appidProp.enums.append("musicPlayer");
        appidProp.enums.append("imageViewer");
        appidProp.enums.append("draw");
        appidProp.enums.append("fileManager");
        appidProp.enums.append("bootMaker");
        appidProp.enums.append("controlCenter");
        appidProp.enums.append("systemMonitor");
        appidProp.enums.append("appstore");
        appidProp.enums.append("devicemanager");
        appidProp.enums.append("logViewer");
        appidProp.enums.append("fontManager");
        appidProp.enums.append("manual");
        appidProp.enums.append("moviePlayer");
        appidProp.enums.append("screenRecorder");
        appidProp.enums.append("terminal");
        appidProp.enums.append("calendar");
        appidProp.enums.append("computer");
        appidProp.enums.append("trash");
        appidProp.enums.append("diskmanager");
        appidProp.enums.append("browser");
        appidProp.enums.append("mail");
        appidProp.enums.append("defender");
        appidProp.enums.append("printerManager");
        appidProp.enums.append("deepinCompressor");
        appidProp.enums.append("uosActivator");
        appidProp.enums.append("calculator");
        appidProp.enums.append("packageInstaller");
        appidProp.enums.append("deepinEditor");
        appidProp.enums.append("deepinReader");
        appidProp.enums.append("deepinAlbum");
        appidProp.enums.append("serviceSupport");
        appidProp.enums.append("introduction");
        appidProp.enums.append("deepinTooltips");
        appidProp.enums.append("recovery");
        appidProp.enums.append("deepinLianliankan");
        appidProp.enums.append("deepinScanner");
        appidProp.enums.append("deepinGomoku");
        appidProp.enums.append("downloader");
        appidProp.enums.append("deepinCamera");
        appidProp.enums.append("deepinVoiceNote");
        appidProp.enums.append("remoteAssistant");
        appidProp.enums.append("chineseimeSetting");
        appidProp.enums.append("cooperation");
        appidProp.enums.append("deepinDataTransfer");
        appidProp.enums.append("mobileAssistant");
        appidProp.enums.append("remmina");
        appidProp.enums.append("wps");
        appidProp.enums.append("excel");
        appidProp.enums.append("word");
        appidProp.enums.append("ppt");
        appidProp.enums.append("pdf");
        appidProp.enums.append("dingtalk");
        appidProp.enums.append("qq");
        appidProp.enums.append("weichat");
        appidProp.enums.append("WeChatWork");
        tool.properties.append(appidProp);

        ModelToolProperty switchProp;
        switchProp.name = "switch";
        switchProp.type = "boolean";
        switchProp.description = "true打开,false关闭";
        tool.properties.append(switchProp);

        tool.required.append("appid");
        tool.required.append("switch");
        tools.append(tool);
    }

    // Tool: systemTotalMemory - 获取系统内存或查看内存
    {
        ModelTool tool;
        tool.name = "systemTotalMemory";
        tool.description = "获取系统内存或查看内存";
        tools.append(tool);
    }

    // Tool: systemFontSize - 设置系统字号
    {
        ModelTool tool;
        tool.name = "systemFontSize";
        tool.description = "设置系统字号，支持11,12,13,14,15,16,18,20";

        ModelToolProperty prop;
        prop.name = "size";
        prop.type = "string";
        prop.description = "字号大小，支持11,12,13,14,15,16,18,20";
        prop.enums.append("11");
        prop.enums.append("12");
        prop.enums.append("13");
        prop.enums.append("14");
        prop.enums.append("15");
        prop.enums.append("16");
        prop.enums.append("18");
        prop.enums.append("20");
        tool.properties.append(prop);

        tool.required.append("size");
        tools.append(tool);
    }

    // Tool: getSystemFontSize - 获取系统字号
    {
        ModelTool tool;
        tool.name = "getSystemFontSize";
        tool.description = "获取当前系统字号大小";
        tools.append(tool);
    }
}

void ToolRegistry::registerFileTools(QList<ModelTool> &tools)
{
    // Tool: openFile - 打开特定文件/文档
    {
        ModelTool tool;
        tool.name = "openFile";
        tool.description = "打开特定文件或文档";

        ModelToolProperty prop;
        prop.name = "filePath";
        prop.type = "string";
        prop.description = "文件路径";
        tool.properties.append(prop);

        tool.required.append("filePath");
        tools.append(tool);
    }

    // Tool: copyFile - 复制文件到指定位置
    {
        ModelTool tool;
        tool.name = "copyFile";
        tool.description = "复制文件到指定位置";

        ModelToolProperty sourceProp;
        sourceProp.name = "sourcePath";
        sourceProp.type = "string";
        sourceProp.description = "源文件路径";
        tool.properties.append(sourceProp);

        ModelToolProperty destProp;
        destProp.name = "destinationPath";
        destProp.type = "string";
        destProp.description = "目标文件路径";
        tool.properties.append(destProp);

        tool.required.append("sourcePath");
        tool.required.append("destinationPath");
        tools.append(tool);
    }

    // Tool: moveFile - 移动文件到指定位置
    {
        ModelTool tool;
        tool.name = "moveFile";
        tool.description = "移动文件到指定位置";

        ModelToolProperty sourceProp;
        sourceProp.name = "sourcePath";
        sourceProp.type = "string";
        sourceProp.description = "源文件路径";
        tool.properties.append(sourceProp);

        ModelToolProperty destProp;
        destProp.name = "destinationPath";
        destProp.type = "string";
        destProp.description = "目标文件路径";
        tool.properties.append(destProp);

        tool.required.append("sourcePath");
        tool.required.append("destinationPath");
        tools.append(tool);
    }

    // Tool: createFolder - 创建新文件夹
    {
        ModelTool tool;
        tool.name = "createFolder";
        tool.description = "创建新文件夹";

        ModelToolProperty prop;
        prop.name = "folderPath";
        prop.type = "string";
        prop.description = "文件夹路径";
        tool.properties.append(prop);

        tool.required.append("folderPath");
        tools.append(tool);
    }

    // Tool: renameFile - 重命名文件
    {
        ModelTool tool;
        tool.name = "renameFile";
        tool.description = "重命名文件";

        ModelToolProperty oldPathProp;
        oldPathProp.name = "oldPath";
        oldPathProp.type = "string";
        oldPathProp.description = "原文件路径";
        tool.properties.append(oldPathProp);

        ModelToolProperty newNameProp;
        newNameProp.name = "newName";
        newNameProp.type = "string";
        newNameProp.description = "新文件名";
        tool.properties.append(newNameProp);

        tool.required.append("oldPath");
        tool.required.append("newName");
        tools.append(tool);
    }

    // Tool: batchRename - 批量重命名多个文件
    {
        ModelTool tool;
        tool.name = "batchRename";
        tool.description = "批量重命名多个文件";

        ModelToolProperty folderPathProp;
        folderPathProp.name = "folderPath";
        folderPathProp.type = "string";
        folderPathProp.description = "文件夹路径";
        tool.properties.append(folderPathProp);

        ModelToolProperty newNameProp;
        newNameProp.name = "newName";
        newNameProp.type = "string";
        newNameProp.description = "新文件名";
        tool.properties.append(newNameProp);

        ModelToolProperty patternProp;
        patternProp.name = "pattern";
        patternProp.type = "string";
        patternProp.description = "正则表达式模式，可选";
        tool.properties.append(patternProp);

        tool.required.append("folderPath");
        tool.required.append("newName");
        tools.append(tool);
    }

    // Tool: readFile - 读取文档内容
    {
        ModelTool tool;
        tool.name = "readFile";
        tool.description = "读取文档内容";

        ModelToolProperty prop;
        prop.name = "filePath";
        prop.type = "string";
        prop.description = "文件路径";
        tool.properties.append(prop);

        tool.required.append("filePath");
        tools.append(tool);
    }

    // Tool: getFileMetadata - 获取文件元数据
    {
        ModelTool tool;
        tool.name = "getFileMetadata";
        tool.description = "获取文件元数据";

        ModelToolProperty prop;
        prop.name = "fileList";
        prop.type = "array";
        prop.description = "文件列表";
        tool.properties.append(prop);

        tool.required.append("fileList");
        tools.append(tool);
    }
}

void ToolRegistry::registerMediaTools(QList<ModelTool> &tools)
{
    // Tool: musicControl - 控制音乐播放
    {
        ModelTool tool;
        tool.name = "musicControl";
        tool.description = "控制音乐播放，支持播放、暂停、上一首、下一首、跳转操作";

        ModelToolProperty prop;
        prop.name = "action";
        prop.type = "string";
        prop.description = "操作类型";
        prop.enums.append("Next");
        prop.enums.append("Previous");
        prop.enums.append("Pause");
        prop.enums.append("Play");
        prop.enums.append("Seek");
        tool.properties.append(prop);

        ModelToolProperty offsetProp;
        offsetProp.name = "offset";
        offsetProp.type = "integer";
        offsetProp.description = "跳转偏移量（秒），仅在action为seek时需要，正数向后跳转，负数向前跳转";
        tool.properties.append(offsetProp);

        tool.required.append("action");
        tools.append(tool);
    }

    // Tool: getBluetoothDevices - 查看已连接的蓝牙设备列表
    {
        ModelTool tool;
        tool.name = "getBluetoothDevices";
        tool.description = "查看已连接的蓝牙设备列表";
        tools.append(tool);
    }
}

void ToolRegistry::registerCommunicationTools(QList<ModelTool> &tools)
{
    // Tool: createSchedule - 创建日程
    {
        ModelTool tool;
        tool.name = "createSchedule";
        tool.description = "创建日程";

        ModelToolProperty subjectProp;
        subjectProp.name = "subject";
        subjectProp.type = "string";
        subjectProp.description = "主题";
        tool.properties.append(subjectProp);

        ModelToolProperty startTimeProp;
        startTimeProp.name = "startTime";
        startTimeProp.type = "string";
        startTimeProp.description = "日程起始时间,格式为yyyy-MM-ddThh:mm:ss";
        tool.properties.append(startTimeProp);

        ModelToolProperty endTimeProp;
        endTimeProp.name = "endTime";
        endTimeProp.type = "string";
        endTimeProp.description = "日程结束时间,格式为yyyy-MM-ddThh:mm:ss";
        tool.properties.append(endTimeProp);

        tool.required.append("subject");
        tool.required.append("startTime");
        tool.required.append("endTime");
        tools.append(tool);
    }

    // Tool: sendMail - 发送邮件
    {
        ModelTool tool;
        tool.name = "sendMail";
        tool.description = "发送邮件";

        ModelToolProperty subjectProp;
        subjectProp.name = "subject";
        subjectProp.type = "string";
        subjectProp.description = "邮件主题";
        tool.properties.append(subjectProp);

        ModelToolProperty contentProp;
        contentProp.name = "content";
        contentProp.type = "string";
        contentProp.description = "邮件正文";
        tool.properties.append(contentProp);

        ModelToolProperty toProp;
        toProp.name = "to";
        toProp.type = "string";
        toProp.description = "收件人";
        tool.properties.append(toProp);

        ModelToolProperty ccProp;
        ccProp.name = "cc";
        ccProp.type = "string";
        ccProp.description = "抄送人";
        tool.properties.append(ccProp);

        ModelToolProperty bccProp;
        bccProp.name = "bcc";
        bccProp.type = "string";
        bccProp.description = "密送人";
        tool.properties.append(bccProp);

        tool.required.append("subject");
        tool.required.append("content");
        tools.append(tool);
    }
}

void ToolRegistry::registerStoreTools(QList<ModelTool> &tools)
{
    // Tool: searchAndInstallApp - 搜索和安装应用商店中的应用
    {
        ModelTool tool;
        tool.name = "searchAndInstallApp";
        tool.description = "当用户要求'安装'某个应用时，应使用此工具搜索应用商店中的应用。搜索后显示应用列表供用户选择，让用户点击具体应用进行下载安装，不要直接安装或自动调用安装接口";

        ModelToolProperty keywordProp;
        keywordProp.name = "keyword";
        keywordProp.type = "string";
        keywordProp.description = "搜索关键词，如'微信'、'浏览器'、'qq'等用户提到的应用名称";
        tool.properties.append(keywordProp);

        tool.required.append("keyword");
        tools.append(tool);
    }
}

} // namespace uos_ai
