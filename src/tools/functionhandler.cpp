#include "functionhandler.h"
#include "stringgenerator.h"
#include "timereventloop.h"
#include "functionsparser.h"
#include "serverwrapper.h"
#include "osinfo.h"

#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QProcess>
#include <QDateTime>

#ifdef FUNCTIONPATH
static const QString &pluginPath = FUNCTIONPATH;
#else
static const QString &pluginPath = "/usr/lib/uos-ai-assistant/functions";
#endif

static const QString &function_split = "___";
static const QString &text2ImageFunName1 = "imageGeneration";
static const QString &text2ImageFunName2 = "drawPicture";
static const int funcitonMaxCount = 10;

bool isValidString(const QString &input)
{
    QRegularExpression regex("[a-zA-Z0-9_-]{1,32}");
    return regex.match(input).hasMatch();
}

QJsonObject FunctionHandler::queryAppFunctions(bool &notYetQueried)
{
    static QJsonObject appFunctions;
    static StringGenerator stringGenerator;

    if (!notYetQueried && !appFunctions.isEmpty())
        return appFunctions;

    notYetQueried = false;

    QDir dir(pluginPath);
    QStringList fileNameList = dir.entryList(QDir::Files);

    for (QString fileName : fileNameList) {
        const QString &id = stringGenerator.generateNextString();
        const QString filePath = dir.filePath(fileName);
        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open file:" << filePath << file.errorString();
            continue;
        }

        QJsonParseError jsonError;
        QByteArray fileData = file.readAll();
        file.close();
        QJsonDocument document = QJsonDocument::fromJson(fileData, &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << filePath << jsonError.errorString();
            continue;
        }

        if (!document.isObject()) {
            qWarning() << "JSON document is not an object:" << fileData;
            continue;
        }

        QJsonArray newFunctions;
        QJsonObject rootObject = document.object();
        const QJsonArray functions = rootObject.value("functions").toArray();

        if (!rootObject.contains("appid")) {
            qWarning() << "Invalid appid:" << rootObject;
            continue;
        }

        if (!rootObject.contains("exec")) {
            qWarning() << "Invalid exec:" << rootObject;
            continue;
        }

        if (!QFile::exists(rootObject.value("exec").toString())) {
            qWarning() << "exec path invalid: " << rootObject.value("exec");
            continue;
        }

        int maxSize = functions.size();
        // 不是助手的，function最大只支持10个
        if (qApp->applicationName() != rootObject.value("appid").toString()) {
            maxSize = qMin(funcitonMaxCount, maxSize);
        }

        for (int i = 0; i < maxSize; i++) {
            QJsonObject function = functions.at(i).toObject();

            if (!function.contains("name") || !function.contains("description") || !function.contains("parameters")) {
                qWarning() << "Invalid function:" << function;
                continue;
            }

            if (!isValidString(function.value("name").toString())) {
                qWarning() << "function name match error [a-zA-Z0-9_-]{1,32} :" << function;
                continue;
            }

            if (function.value("description").toString().length() > 30) {
                qWarning() << "function description maximum length reached 30 :" << function.value("description");
                continue;
            }

            QString funName = function["name"].toString();
            if (funName == "createSchedule") {
                function["description"] = function["description"].toString().arg(QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss"));
            }

            function["name"] = id + function_split + funName;
            newFunctions << function;
        }

        rootObject["functions"] = newFunctions;
        appFunctions[id] = rootObject;
    }

    return appFunctions;
}

QJsonArray FunctionHandler::functions(const QJsonObject &appFunctions)
{
    QJsonArray functions;
    for (const auto &functionObject : appFunctions) {
        const QJsonArray &fs = functionObject.toObject().value("functions").toArray();
        for (const QJsonValue &functionValue : fs) {
            functions << functionValue;
        }
    }

    return functions;
}

QString FunctionHandler::functionPluginPath()
{
    return pluginPath;
}

QJsonObject FunctionHandler::functionProcess(const QJsonObject &appFunctions, QJsonObject fun, QString *directReply)
{
    const QString &name = fun.value("name").toString();
    const QStringList &funName = name.split(function_split);
    fun["name"] = funName.value(1);

    bool execResult = false;
    QString outputString = QCoreApplication::translate("ChatSeesion", "function parsing failed");
    const QJsonObject &appFunctionObj = appFunctions.value(funName.value(0)).toObject();
    QString appId = appFunctionObj.value("appid").toString();
    if (qApp->applicationName()  == appId) {
        QEventLoop oneloop;
//        oneloop.setTimeout(5000);

        FunctionsParser funParser(fun);
        QObject::connect(&funParser, &FunctionsParser::finished, &oneloop, &QEventLoop::quit);
        funParser.start();

        oneloop.exec();

        execResult = funParser.exitCode() == 0 ? true : false;
        outputString = funParser.outputString();

        if (directReply)
            *directReply = funParser.directOutput();
    } else {
        ServerWrapper::instance()->addAppFunction(appId, fun);
        QString exec = appFunctionObj.value("exec").toString();

        if (!QFile::exists(exec)) {
            outputString = QCoreApplication::translate("ChatSeesion", "Application file does not exist");
            qWarning() << "exec path invalid: " << exec;
        } else {
            QProcess process;
            process.setProcessEnvironment(UosInfo()->pureEnvironment());
            process.setArguments(QStringList() << "--functioncall");
            process.setProgram(exec);
            qint64 pid = 0;
            execResult = process.startDetached(&pid);
            if (!execResult)
                outputString = process.errorString();
            else
                outputString.clear();

            qInfo() << "start process " << exec  << "pid" << pid << process.exitStatus()
                    << outputString;
            qDebug() << "with environment" << UosInfo()->pureEnvironment().toStringList();
        }
    }

    QJsonObject resArguments;
    if (execResult) {
        resArguments["description"] = QCoreApplication::translate("ChatSeesion", "Started successfully");
    } else {
        resArguments["description"] = QCoreApplication::translate("ChatSeesion", "Startup failed");
    }

    if (!outputString.isEmpty()) {
        resArguments["description"] = resArguments["description"].toString() + QCoreApplication::translate("ChatSeesion", " The execution output content is ") + outputString ;
    }

    return resArguments;
}

QJsonArray FunctionHandler::functionCall(const QJsonObject &response, const QString &conversation, QString *directReply)
{
    bool query = false;
    QJsonObject appFunctions = FunctionHandler::queryAppFunctions(query);

    QJsonArray functionCalls;
    QJsonArray tmpconversion =  QJsonDocument::fromJson(conversation.toUtf8()).array();
    if (!tmpconversion.isEmpty()) {
        functionCalls = tmpconversion;
    } else {
        functionCalls << QJsonObject({
            {"role", "user"},
            {"content", conversation}
        });
    }

    const QJsonObject &tools = response.value("tools").toObject();
    if (tools.contains("function_call")) {
        const QJsonObject &fun = tools.value("function_call").toObject();

        functionCalls << QJsonObject({
            {"role", "assistant"},
            {"function_call", fun},
            {"content", QJsonValue::Null}
        });

        const QJsonObject &resArguments = functionProcess(appFunctions, fun, directReply);

        functionCalls << QJsonObject({
            {"role", "function"},
            {"name", fun.value("name")},
            {"content", QString(QJsonDocument(resArguments).toJson(QJsonDocument::Compact))}
        });

        return functionCalls;
    }

    if (tools.contains("tool_calls")) {
        functionCalls << QJsonObject({
            {"role", "assistant"},
            {"content", response.value("content")},
            {"tool_calls", tools.value("tool_calls")},
        });

        const QJsonArray &tool_calls = tools.value("tool_calls").toArray();

        for (const QJsonValue &tool_call : tool_calls) {
            const QJsonObject &fun = tool_call["function"].toObject();
            const QJsonObject &resArguments = functionProcess(appFunctions, fun, directReply);

            functionCalls << QJsonObject({
                {"role", "tool"},
                {"tool_call_id", tool_call["id"]},
                {"name", fun.value("name")},
                {"content", resArguments.value("description")}
            });
        }

        return functionCalls;
    }

    qWarning() << "functionCall error = " << response;
    return QJsonArray();
}

int FunctionHandler::chatAction(const QJsonObject &response)
{
    if (response.contains("tools")) {
        const QJsonObject &tools = response.value("tools").toObject();

        if (tools.contains("tool_calls")) {
            const QJsonArray &tool_calls = tools.value("tool_calls").toArray();
            for (const QJsonValue &tool_call : tool_calls) {
                if (tool_call["type"] != "function")
                    continue;

                QString funName = tool_call["function"]["name"].toString();
                if (funName.endsWith(text2ImageFunName1) || funName.endsWith(text2ImageFunName2)) {
                    return ChatText2Image;
                }
            }
        }

        if (tools.contains("function_call")) {
            QString funName = tools.value("function_call")["name"].toString();
            if (funName.endsWith(text2ImageFunName1) || funName.endsWith(text2ImageFunName2))
                return ChatText2Image;
        }

        return ChatFunctionCall;
    }

    return ChatTextPlain;
}
