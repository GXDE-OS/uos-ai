// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef OAFUNCTIONHANDLER_H
#define OAFUNCTIONHANDLER_H

#include <QObject>
#include <QJsonArray>
#include <QMap>

namespace uos_ai {
namespace rpa {

class OaFunctionHandler : public QObject
{
    Q_OBJECT
public:
    static OaFunctionHandler *instance();

    static QJsonArray queryFunctions();

    QString OaFunctionCall(const QJsonObject &func);

    enum TaskStatus {
        TaskOngoing     = 1, // 进行中
        TaskAwaiting    = 2, // 待进行
        TaskFailed      = 3, // 失败
        TaskFinished    = 4, // 完成
        TaskTimeout     = 5, // 超时
        TaskSkipped     = 6, // 跳过
    };
private:
    OaFunctionHandler();

    QString OaApplyBusinessTrip(const QJsonObject &argumentsObj);
    QString OaApplyMeeting(const QJsonObject &argumentsObj);
    QString OaApplyLeave(const QJsonObject &argumentsObj);
    QString OaApplyOvertime(const QJsonObject &argumentsObj);
    QString OaApplyReimbursement(const QJsonObject &argumentsObj);

private:
    using HeadersData = QPair<QByteArray, QByteArray>;
    QJsonObject onHttpRequest(const QString &url, const QList<HeadersData> &headers, const QJsonObject &data);
    QJsonObject onGetRequest(const QString &url, const QList<HeadersData> &headers);
    QString getAccessToken();
    int getProjectId(const QString &processId);
    void getRobotId();
    QJsonObject sendTaskRequest(const QString &processID, const QJsonObject &param);
    QString getTaskResult(const QJsonObject& taskRes);

    QString m_accessToken;
    QString m_robotUuid;
    int m_robotID;
    QMap<QString, int> m_project;

};
}}
#endif // OAFUNCTIONHANDLER_H
