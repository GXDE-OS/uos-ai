// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "oafunctionhandler.h"

#include <QElapsedTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QStandardPaths>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QThread>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

using namespace uos_ai;
using namespace uos_ai::rpa;

static const char *secretId = "ffa138de14fe4c9f8181220364a1c878";
static const char *secretKey = "57f6577506fe4c9f8181220364a1c878";
static const char *apiUrl = "https://api.pre.ninetechone.com/api/controller/v1/4j";

OaFunctionHandler *OaFunctionHandler::instance()
{
    static OaFunctionHandler ins;
    return &ins;
}

OaFunctionHandler::OaFunctionHandler()
{
    m_accessToken = getAccessToken();

    QByteArray data;
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/robot_Config.txt";
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "config file don't exits";
    } else {
        data = file.readAll();
    }

    file.close();

    if (!data.isEmpty()) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isEmpty())
            qWarning() << "json parser error!";
        else {
            QJsonObject obj = jsonDoc.object();
            m_robotUuid = obj.value("robot_uuid").toString();
        }
    }
    qInfo() << "robot uuid:" << m_robotUuid;

    getRobotId();
}

QJsonArray OaFunctionHandler::queryFunctions()
{
#ifdef QT_DEBUG
    QString funcsPath = QString(ASSETS_INSTALL_DIR) + "functions/org.uos.ai.oa.json";
#else
    QString funcsPath = QString(ASSETS_INSTALL_DIR) + "org.uos.ai.oa.json";
#endif
    qDebug() << "funcsPath:" << funcsPath;

    QFile file(funcsPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file:" << funcsPath << file.errorString();
        return QJsonArray();
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonArray newFuncs;
    QJsonArray funcArray = QJsonDocument::fromJson(fileData).array();
    for (QJsonValue func : funcArray) {
        QJsonObject funcObj = func.toObject();
        QString funcName = funcObj.value("name").toString();

        if (funcName == "supplyBusinessTrip" || funcName == "applyMeeting" || funcName == "applyLeave"
                || funcName == "applyOvertime" || funcName == "applyReimbursement") {
            funcObj["description"] = funcObj["description"].toString().arg(QDateTime::currentDateTime().toString("yyyy-MM-dd"));
        }

        newFuncs << funcObj;
    }

    return newFuncs;
}

QString OaFunctionHandler::OaFunctionCall(const QJsonObject &func)
{
    QString resUrl;

    QJsonObject argObj = QJsonDocument::fromJson(func.value("arguments").toString().toUtf8()).object();
    QString funcName = func.value("name").toString();

    if (funcName == "supplyBusinessTrip") {
        resUrl = OaApplyBusinessTrip(argObj);
    } else if (funcName == "applyMeeting") {
        resUrl = OaApplyMeeting(argObj);
    } else if (funcName == "applyLeave") {
        resUrl = OaApplyLeave(argObj);
    } else if (funcName == "applyOvertime") {
        resUrl = OaApplyOvertime(argObj);
    } else if (funcName == "applyReimbursement") {
        resUrl = OaApplyReimbursement(argObj);
    }

    return resUrl;
}

// 出差申请
QString OaFunctionHandler::OaApplyBusinessTrip(const QJsonObject &argumentsObj)
{
    QString processID("1820761128755630081");

    QJsonObject sendDataObj = argumentsObj;
    QJsonArray travelInformation;
    QJsonObject startTIObj;  // 出发信息
    QJsonObject endTIObj;    // 回来信息

    if (sendDataObj.contains("StartDate"))
        startTIObj.insert("TravelDate", sendDataObj.value("StartDate").toString());
    if (sendDataObj.contains("EndDate"))
        endTIObj.insert("TravelDate", sendDataObj.value("EndDate").toString());
    if (sendDataObj.contains("departure")) {
        startTIObj.insert("PlaceOfDeparture", sendDataObj.value("departure").toString());
        endTIObj.insert("Destination", sendDataObj.value("departure").toString());

        startTIObj.insert("Takeoff", sendDataObj.value("departure").toString());
        endTIObj.insert("Takeon", sendDataObj.value("departure").toString());
    }
    if (sendDataObj.contains("destination")) {
        startTIObj.insert("Destination", sendDataObj.value("destination").toString());
        endTIObj.insert("PlaceOfDeparture", sendDataObj.value("destination").toString());

        startTIObj.insert("Takeon", sendDataObj.value("destination").toString());
        endTIObj.insert("Takeoff", sendDataObj.value("destination").toString());
    }
    if (sendDataObj.contains("transportation")) {
        startTIObj.insert("Vehicle", sendDataObj.value("transportation").toString());
        endTIObj.insert("Vehicle", sendDataObj.value("transportation").toString());
    }
    if (sendDataObj.contains("HotelReservationMethod")) {
        startTIObj.insert("HotelReservationMethod", sendDataObj.value("HotelReservationMethod").toString());
        endTIObj.insert("HotelReservationMethod", sendDataObj.value("HotelReservationMethod").toString());
    }
    travelInformation.append(startTIObj);
    travelInformation.append(endTIObj);
    sendDataObj.insert("TravelInformation", travelInformation);
    sendDataObj.insert("ArrangeAccommodation", "否");

    return getTaskResult(sendTaskRequest(processID, sendDataObj));
}

// 会议申请
QString OaFunctionHandler::OaApplyMeeting(const QJsonObject &argumentsObj)
{
    QString processID("1829128042422833154");
    qDebug() << "OaApplyMeeting:" << processID;

    QJsonObject sendDataObj = argumentsObj;

    return getTaskResult(sendTaskRequest(processID, sendDataObj));
}

// 请假申请
QString OaFunctionHandler::OaApplyLeave(const QJsonObject &argumentsObj)
{
    QString processID("1826309272184655873");
    qDebug() << "OaApplyLeave:" << processID;

    QJsonObject sendDataObj = argumentsObj;

    return getTaskResult(sendTaskRequest(processID, sendDataObj));
}

// 加班申请
QString OaFunctionHandler::OaApplyOvertime(const QJsonObject &argumentsObj)
{
    QString processID("1828689872455569409");
    qDebug() << "OaApplyOvertime:" << processID;

    QJsonObject sendDataObj = argumentsObj;

    return getTaskResult(sendTaskRequest(processID, sendDataObj));
}

// 差旅费报销
QString OaFunctionHandler::OaApplyReimbursement(const QJsonObject &argumentsObj)
{
    QString processID("1829337016870998018");

    QJsonObject sendDataObj = argumentsObj;
    QJsonArray travelInformation;
    QJsonObject startTIObj;  // 出发信息
    QJsonObject endTIObj;    // 回来信息

    if (sendDataObj.contains("StartDate"))
        startTIObj.insert("TravelDate", sendDataObj.value("StartDate").toString());
    if (sendDataObj.contains("EndDate"))
        endTIObj.insert("TravelDate", sendDataObj.value("EndDate").toString());
    if (sendDataObj.contains("departure")) {
        startTIObj.insert("PlaceOfDeparture", sendDataObj.value("departure").toString());
        endTIObj.insert("Destination", sendDataObj.value("departure").toString());
    }
    if (sendDataObj.contains("destination")) {
        startTIObj.insert("Destination", sendDataObj.value("destination").toString());
        endTIObj.insert("PlaceOfDeparture", sendDataObj.value("destination").toString());
    }

    travelInformation.append(startTIObj);
    travelInformation.append(endTIObj);
    sendDataObj.insert("TravelInformation", travelInformation);

    return getTaskResult(sendTaskRequest(processID, sendDataObj));
}

QString OaFunctionHandler::getTaskResult(const QJsonObject& taskRes)
{
    if (taskRes.contains("code") && taskRes.value("code") != "200") {
        QString res = taskRes.value("message").toString();
        qWarning() << res;
        return res;
    }

    int timingId = taskRes.value("data").toObject().value("timingId").toInt();
    if (timingId == -1)
        return QString("定时任务ID 无效!");

    QString taskResult;

    QString url = apiUrl + QString("/open/api/timing/get-tasks");

    QList<HeadersData> headers;
    HeadersData header = qMakePair(QString("Access-token").toUtf8(), m_accessToken.toUtf8());
    headers.append(header);

    QJsonObject data;
    data.insert("timingId", timingId);
    QJsonObject requestRes;

    int status = TaskTimeout;  // 初始化为超时状态
    QThread thread;

    // 设定一个180s秒超时时间，如果流程在3分钟内未能结束，认为超时
    QElapsedTimer timer;
    timer.start();  // 启动计时器
    while (timer.elapsed() < 180 * 1000) {  // 180秒 * 1000毫秒
        qInfo() << "waiting...";

        thread.sleep(5);

        requestRes = onHttpRequest(url, headers, data);
        if (taskRes.contains("code") && taskRes.value("code") != "200") {
            QString res = taskRes.value("message").toString();
            qWarning() << res;
            return res;
        }
        status = requestRes.value("data").toArray()[0].toObject().value("status").toInt();
        qInfo() << "task status:" << status;
        if (status == TaskOngoing || status == TaskAwaiting) {
            continue;
        } else if (status == TaskFinished) {
            break;
        } else {
            break;
        }
    }

    if (status == TaskOngoing || status == TaskAwaiting) {
        qWarning() << "timeout!";
        return "任务超时！";
    } else if (status == TaskFinished) {
        QString outData = requestRes.value("data").toArray()[0].toObject().value("flowResultValue").toObject().value("outData").toString();
        QJsonDocument docOutdata;
        QString url = docOutdata.fromJson(outData.toUtf8()).object().value("url").toString();
        taskResult = QString("操作成功，⼀切顺利！请查看表单： [%1](%2)").arg(url).arg(url);
        qDebug() << "taskResult:" << taskResult;
    } else if (status == TaskFailed) {
        return "任务执行失败！";
    } else if (status == TaskSkipped) {
        return "任务跳过!";
    }

    return taskResult;
}

QJsonObject OaFunctionHandler::onHttpRequest(const QString &url, const QList<HeadersData> &headers, const QJsonObject &data)
{
    QJsonDocument doc(data);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    for (const HeadersData &header : headers) {
        request.setRawHeader(header.first, header.second);
    }

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, doc.toJson(QJsonDocument::Compact));
    bool timeoutFlag = false;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(60 * 1000);
    timer.start();

    QObject::connect(&timer, &QTimer::timeout, &loop, [&loop, &timeoutFlag]() {
        timeoutFlag = true;
        loop.quit();
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    loop.exec();
    timer.stop();

    reply->deleteLater();

    if (timeoutFlag) {
        return QJsonDocument::fromJson("{\"code\":\"201\",\"message\":\"timeout\"}").object();
    } else {
        return QJsonDocument::fromJson(reply->readAll()).object();
    }
}

QJsonObject OaFunctionHandler::onGetRequest(const QString &url, const QList<OaFunctionHandler::HeadersData> &headers)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    for (const HeadersData &header : headers) {
        request.setRawHeader(header.first, header.second);
    }

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(60 * 1000);
    timer.start();

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    loop.exec();
    timer.stop();

    reply->deleteLater();

    return QJsonDocument::fromJson(reply->readAll()).object();
}

QString OaFunctionHandler::getAccessToken()
{
    QString url = apiUrl + QString("/open/api/accessToken");
    QString accessSecret = "secretId:%1 secretKey:%2";
    QByteArray accessSecretData = accessSecret.arg(secretId).arg(secretKey).toUtf8().toBase64();

    QList<HeadersData> headers;
    HeadersData header = qMakePair(QString("Access-secret").toUtf8(), accessSecretData);
    headers.append(header);

    QJsonObject sendData;
    sendData.insert("isForce", true);

    QJsonObject requestRes = onHttpRequest(url, headers, sendData);
    if (requestRes.value("code") != "200") {
        qWarning() << requestRes.value("message").toString();
        return QString();
    }
    QString accessToken = requestRes.value("data").toObject().value("accessToken").toString();

    return accessToken;
}

int OaFunctionHandler::getProjectId(const QString &processId)
{
    QString url = apiUrl + QString("/open/api/processes/version?page=1&size=50&processId=%1").arg(processId);

    QList<HeadersData> headers;
    HeadersData header = qMakePair(QString("Access-token").toUtf8(), m_accessToken.toUtf8());
    headers.append(header);

    QJsonObject requestRes = onGetRequest(url, headers);
    if (requestRes.value("code") != "200") {
        qWarning() << requestRes.value("message").toString();
         return 0;
    }
    int projectId = requestRes.value("data").toObject().value("records").toArray()[0].toObject().value("id").toInt();

    return projectId;
}

void OaFunctionHandler::getRobotId()
{
    QString url = apiUrl + QString("open/api/describeRobots");

    QList<HeadersData> headers;
    HeadersData header = qMakePair(QString("Access-token").toUtf8(), m_accessToken.toUtf8());
    headers.append(header);

    int page = 0;

    while (true) {
        page += 1;
        QJsonObject data;
        QJsonArray status;
        status.append(QJsonValue(QString("RUNNING")));
        status.append(QJsonValue(QString("PAUSE")));
        status.append(QJsonValue(QString("IDLE")));
        status.append(QJsonValue(QString("DISCONNECT")));
        data.insert("statuses", status);
        data.insert("page", QString::number(page));

        //QString jsonData = QJsonDocument(data).toJson(QJsonDocument::Compact);

        QJsonObject requestRes = onHttpRequest(url, headers, data);
        if (requestRes.value("code") != "200") {
            qWarning() << requestRes.value("message").toString();
            return;
        }

        QJsonArray recordArray = requestRes.value("data").toObject().value("records").toArray();

        for (auto record : recordArray) {
            if (m_robotUuid == record.toObject().value("uuid").toString()) {
                m_robotID = record.toObject().value("id").toInt();
                return;
            }
        }

        int total = requestRes.value("data").toObject().value("total").toInt();
        if (page >= total) {
            qWarning() << "robot not found!";
            return;
        }
    }
}

QJsonObject OaFunctionHandler::sendTaskRequest(const QString &processID, const QJsonObject &param)
{
    QString url = apiUrl + QString("/open/api/createTask");

    QList<HeadersData> headers;
    HeadersData header = qMakePair(QString("Access-token").toUtf8(), m_accessToken.toUtf8());
    headers.append(header);

    int projectId;
    if (m_project.contains(processID))
        projectId = m_project.value(processID);
    else {
        projectId = getProjectId(processID);
        m_project.insert(processID, projectId);
    }

    QJsonObject data;
    data.insert("projectId", projectId);
    data.insert("robotId", m_robotID);
    data.insert("isTiming", false);
    QJsonObject task;
    task.insert("taskData", QString::fromUtf8(QJsonDocument(param).toJson(QJsonDocument::Compact)));
    data.insert("parameters", task);

    return onHttpRequest(url, headers, data);
}
