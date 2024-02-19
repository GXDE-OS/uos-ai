#ifndef DEEPINCALENDAR_H
#define DEEPINCALENDAR_H
#include "scheduleability.h"

#include <QObject>
#include <QVariant>
#include <QJsonObject>

class DeepinCalendar : public QObject, public ISchedule
{
    Q_OBJECT
public:
    explicit DeepinCalendar(QObject *parent = nullptr);
    virtual ~DeepinCalendar();

    enum class ScheduleView {
        SVNone = 0,
        SVYear = 1,
        SVMonth = 2,
        SVWeek = 3,
        SVDay = 4,
        SVMax
    };

    enum class ScheduleViewType {
        SVTNone = 0,
        SVTNormal = 1,
        SVTEmptyJob = 2,
        SVTMax
    };

    enum class RemindSetting {
        RMSNone = 0,                 // 不提醒
        RMSWhenHappen = 1,           // 发生时提醒
        RMSBeforeFifteenMinute = 2,  // 提前15分钟提醒
        RMSBeforeThirtyMinute = 3,   // 提前30分钟提醒
        RMSBeforeOneHour = 4,        // 提前1小时提醒
        RMSBeforeOneDay = 5,         // 提前1天提醒
        RMSBeforeTwoDay = 6,         // 提前2天提醒
        RMSBeforeOneWeek = 7,        // 提前1周提醒
        RMSMax
    };

    enum class RepeatSetting {
        RPSNone = 0,                 // 不提醒
        RPSEveryDay = 1,             // 发生时提醒
        RPSWorkingDay = 2,           // 工作日提醒
        RPSEveryWeek = 3,            // 每周提醒
        RPSEveryMonth = 4,           // 每月提醒
        RPSEveryYear = 5,            // 每年提醒
        RPSMax
    };

    virtual int createSchedule(const QJsonObject &objCreate);
    virtual int viewSchedule(const QJsonObject &objView);
    virtual int querySchedule(const QJsonObject &objQuery, QVariant &retQuery);
    virtual int cancelSchedule(const QJsonObject &objCancel);
signals:
private:
    virtual int invoke(const QString &intent, const QJsonObject &objSchedule, QVariant &retInvoke);

};

#endif // DEEPINCALENDAR_H
