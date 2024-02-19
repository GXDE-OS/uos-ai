#ifndef SCHEDULEABILITY_H
#define SCHEDULEABILITY_H

#define SCHEDULEABILITY_H

#include <QVariant>
#include <QJsonObject>

class ISchedule
{
public:
    virtual ~ISchedule() {}
    virtual int createSchedule(const QJsonObject &objCreate) = 0;
    virtual int viewSchedule(const QJsonObject &objView) = 0;
    virtual int querySchedule(const QJsonObject &objQuery, QVariant &retQuery) = 0;
    virtual int cancelSchedule(const QJsonObject &objCancel) = 0;
};

#endif // SCHEDULEABILITY_H
