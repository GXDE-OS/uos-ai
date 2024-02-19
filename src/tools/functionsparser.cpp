#include "functionsparser.h"
#include "deepinabilitymanager.h"

#include <QTimer>
#include <QDebug>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QDesktopServices>
#include <QProcess>
#include <QDBusInterface>

FunctionsParser::FunctionsParser(const QJsonObject &function)
    : QThread(nullptr)
    , m_function(function)
{

}

FunctionsParser::~FunctionsParser()
{

}

int FunctionsParser::exitCode()
{
    return m_exitCode;
}

QString FunctionsParser::outputString()
{
    return m_outputString;
}

void FunctionsParser::sendMailFunction(const QJsonObject &argumentsObj)
{
    QString subject = argumentsObj.value("subject").toString();
    QString content = argumentsObj.value("content").toString();
    QStringList toList = argumentsObj.value("to").toString().split(",");
    QStringList ccList = argumentsObj.value("cc").toString().split(",");
    QStringList bccList = argumentsObj.value("bcc").toString().split(",");

    // 创建一个mailto链接
    QUrl mailtoUrl;
    QUrlQuery queryItem;
    mailtoUrl.setScheme("mailto");

    // 设置收件人
    if (!toList.join(",").isEmpty()) {
        queryItem.addQueryItem("to", toList.join(","));
    }

    // 设置抄送人
    if (!ccList.join(",").isEmpty()) {
        queryItem.addQueryItem("cc", ccList.join(","));
    }

    // 设置密送人
    if (!bccList.join(",").isEmpty()) {
        queryItem.addQueryItem("bcc", bccList.join(","));
    }

    // 设置主题
    if (!subject.isEmpty()) {
        queryItem.addQueryItem("subject", QUrl::toPercentEncoding(subject));
    }

    // 设置正文
    if (!content.isEmpty()) {
        queryItem.addQueryItem("body", QUrl::toPercentEncoding(content));
    }

    queryItem.addQueryItem("type", "1");

    mailtoUrl.setQuery(queryItem);
    openUrl(mailtoUrl.toString());
}

void FunctionsParser::openUrl(const QString &url)
{
    QProcess process;
    QString command = QString("xdg-open \"%1\"").arg(url);
    process.start(command);
    process.waitForFinished(5000);
    int exitCode = process.exitCode();

    QString outputString;
    QString standardOutput = process.readAllStandardOutput();
    QStringList outLst = standardOutput.split(":");
    if (!outLst.isEmpty())
        outputString = outLst.last();

    if (outputString.isEmpty())
        outputString = standardOutput;

    m_exitCode = exitCode;
    m_outputString = outputString;

    if (exitCode != 0)
        qWarning() << url << "Exit code: " << exitCode << "Standard Output: " << outputString;
}

void FunctionsParser::openExec(const QString &mimeType)
{
    QProcess process;
    QString command = QString("xdg-mime query default %1").arg(mimeType);
    process.start(command);
    process.waitForFinished(5000);
    m_exitCode = process.exitCode();

    QString outputString;
    QStringList outLst = QString(process.readAllStandardOutput()).split(":");
    if (!outLst.isEmpty())
        outputString = outLst.last();

    m_outputString = outputString;
    qWarning() << "Exit code: " << m_exitCode << "Standard Output: " << outputString;

    if (m_exitCode != 0)
        return;

    outputString = outputString.trimmed();
    if (!outputString.endsWith(".desktop")) {
        m_exitCode = 1;
        qWarning() << "FunctionsParser::openExec " << outputString;
        return;
    }

    QDBusInterface notification("com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager", QDBusConnection::sessionBus());
    QList<QVariant> args;
    args.append("/usr/share/applications/" + outputString);
    QString error = notification.callWithArgumentList(QDBus::Block, "Launch", args).errorMessage();
    if (!error.isEmpty()) {
        m_exitCode = 1;
        m_outputString = error;
        qCritical() << error;
    }
}

void FunctionsParser::run()
{
    m_exitCode = 0;
    m_outputString.clear();

    OSCallContext content;
    content.error = OSCallContext::NonError;

    const QString &funName = m_function.value("name").toString();
    const QJsonObject &argumentsObj = QJsonDocument::fromJson(m_function.value("arguments").toString().toUtf8()).object();
    if (funName == "sendMail") {
        sendMailFunction(argumentsObj);
    } else if (funName == "openBluetooth") {
        content = UosAbility()->doBluetoothConfig();
    } else if (funName == "openScreenMirroring") {
        content = UosAbility()->doScreenMirroring();
    } else if (funName == "switchNoDisturbMode") {
        content = UosAbility()->doNoDisturb(argumentsObj.value("switch").toBool());
    } else if (funName == "switchWallpaper") {
        content = UosAbility()->doWallpaperSwitch();
    } else if (funName == "clearDesktop") {
        content = UosAbility()->doDesktopClearing(argumentsObj.value("switch").toBool());
    } else if (funName == "switchDockMode") {
        QString modestr = argumentsObj.value("mode").toString();
        int mode = -1;
        if (modestr == "Fashion")
            mode = 0;
        else if (modestr == "Efficent")
            mode = 1;
        else
            qWarning() << "function args error = " << argumentsObj;

        content = UosAbility()->doDockModeSwitch(mode);
    } else if (funName == "switchSystemTheme") {
        QString modestr = argumentsObj.value("theme").toString();
        int mode = -1;
        if (modestr == "Light")
            mode = 0;
        else if (modestr == "Dark")
            mode = 1;
        else if (modestr == "Auto")
            mode = 2;
        else
            qWarning() << "function args error = " << argumentsObj;

        content = UosAbility()->doSystemThemeSwitch(mode);
    } else if (funName == "switchEyesProtection") {
        content = UosAbility()->doDiplayEyesProtection(argumentsObj.value("switch").toBool());
    } else if (funName == "displayBrightness") {
        int percent = argumentsObj.value("percent").toInt();
        content = UosAbility()->doDiplayBrightness(percent);
    } else if (funName == "launchApplication") {
        QString appId = argumentsObj.value("appid").toString();
        bool on = argumentsObj.value("switch").toBool();
        content = UosAbility()->doAppLaunch(appId, on);
    } else if (funName == "createSchedule") {
        QString subject   = argumentsObj.value("subject").toString();
        QString startTime = argumentsObj.value("startTime").toString();
        QString endTime   = argumentsObj.value("endTime").toString();
        content = UosAbility()->doCreateSchedule(subject, startTime, endTime);
    } else {
        content.error = OSCallContext::NotImpl;
        content.errorInfo = funName + QCoreApplication::translate("FunctionsParser", "Function not available");

        qWarning() << "function name error = " << m_function;
    }

    if (content.error != OSCallContext::NonError) {
        m_exitCode = content.error;
        m_outputString = content.errorInfo;
    }
}
