/*
 * 日历规范较为复杂，调用仅提供一个接口invoke，完成所有日历操作
 *
 * 第一个参数代表意图，分别是CREATE、QUERY、VIEW、CANCEL
 * 第二参数为格式化json字符串:
    {
        "ID": 1,
        "Title": "去武汉",
        "AllDay": false,
        "Description": "原因",
        "Type": 1,
        "Start": "2020-01-02T15:04:05+07:00",
        "End": "2020-01-06T15:04:05+07:00",
        "RecurID": 0,
        "Ignore": ["2020-01-02T15:04:05+07:00", "2020-01-02T15:04:05+07:00"],
        "Remind": "",
        "RRule": "",

        "ViewName": "1",        //视图名称:year,month,week,day对应年、月、周、日视图类型
        "ViewTime": \"2019-10-23T10:00:00\",    //表示需要查看视图上某个时间

        "ADTitleName":"去",
        "ADStartTime":\"2019-10-23T10:00:00\",
        "ADEndTime":\"2019-10-23T10:00:00\"
    }

    ## ID
    数据类型 int

    ## AllDay 提醒是否为全天提醒
    数据类型 bool

    ## Remind 提醒的提前时间，提醒和是否全天有关，不提醒置空，提醒的提前时间长度最长为7天。
    数据类型: string

    全天时：
    - 指定提前的天数和时间 —— 格式 "n;hh:mm", 比如 "1;08:00", 表示提醒时间为1天前的 08:00。
    非全天时：
    - 指定提前的分钟数 —— 格式 "n", 比如 "0" 表示不提前， 而”5" 表示提前5分钟提醒， "60" 表示提前 1 小时。

    ## RRule 重复规则，重复规则表达式就是 RFC 5545 中规定的 RRule
    数据类型: string

    - 是用分号分隔的键值对,比如 `FREQ=DAILY;INTERVAL=3;COUNT=3`。
    - FREQ:  事件重复频率，事件重复频率，有效值：DAILY(按天)，WEEKLY(按周)，MONTHLY(按月)，YEARLY(按年)
    - INTERVAL: 事件重复的间隔，如按天重复时，INTERVAL=2，表示每2天重复一次，默认值为1。
    - COUNT: 事件重复多少次后结束，该字段和 UNTIL 字段两者只能出现一个。
    - UNTIL: 结束重复时间，格式如 20130102T170000Z，表示 2013-1-2 下午5点结束。
    - BYDAY: 表示一周的某一天，有效值：MO(周一),TU(周二),WE(周三),TH(周四),FR(周五),SA(周六),SU(周日) ， 示例： BYDAY=MO,TH,SU 表示重复日期包括周一，周四，周日. 每个值前面可以用 ”+”, “-” 修饰,表示第几个和倒数第几个日子,如 BYDAY = 2MO 表示第2个星期一发生; BYDAY=MO,-1SU 表示每个星期一和最后一个星期日发生
    - BYMONTHDAY: 表示一月的第几天发生,有效值是 [1 ~ 31] 和 [-31 ~ -1] ,如: BYMONTHDAY=2,18 表示一月的第2天,第18天发生; BYMONTHDAY=-1 表示一月的最后一天
    - BYYEARDAY: 表示一年的第几天发生,有效值是 [1 ~ 366] 和 [-366 ~ -1], 如 BYYEARDAY=125 表示一年的第125年发生; BYYEARDAY=-1 表示一年的最后一天发生
    - BYWEEKNO: 表示一年的第几周发生, 有效值是 [1 ~ 53] 和 [-53 ~ -1], 如 BYWEEKNO=2,23 表示一年的第2周,第23周发生
    - BYMONTH: 表示一年中的第几个月发生, 有效值是 [1 ~ 12]

    需要注意几点:
    - 如果各字段所设置的值是无效的,如 BYMONTHDAY=30 ,则会忽略该值
    - 如果某条事件的重复规则表达式缺少一些必要字段,如 YEARLY;BYMONTH=1 ,表示按年重复,每年的1月某日发生,现在缺少”日”字段,则从该事件的”开始日期”中获得

    备注：重复为“每天”的规则是 `FREQ=DAILY`，重复为“每个工作日”的规则是 `FREQ=DAILY;BYDAY=MO,TU,WE,TH,FR`，重复为“每周”的规则是`FREQ=WEEKLY`，重复为“每月”的规则是`FREQ=MONTHLY`，重复为“每年”的规则是`FREQ=YEARLY`，如果 UI 上设置重复 N 次，则 COUNT=N+1。
    备注：有关重复截至条件日期的设置，如果 job 开始时间为 2019-09-01 09:00，重复：每日，截至期为 2019-09-03，则需要设置 UNTIL=20190902T090000Z。
    已知截至日期，求 UNTIL 值的计算方法是截至日期 - 1 天，然后时分秒设置为 job 的开始时间的时分秒。
    已知 UNTIL 值，求截至日期的计算方法是 UNTIL 值中的日期 + 1 天。

    ## Title 标题
    数据类型: string

    ## Description 描述
    数据类型: string

    ## Type 类型
    数据类型：int
    1为Work 2 Life 3 Other

    ## RecurID 重复性ID
    数据类型 int

    数值等同于重复了几次才到这个 job。只读，根据重复规则计算出的。
    重复性的 job 的本体的 RecurID 为 0，第一个复制体的 RecurID 为 1。


    ## Start 开始时间
    数据类型: string

    ## End 结束时间
    数据类型: string

    - 时间格式为 RFC3339，比如"2006-01-02T15:04:05+07:00"。

    ## Ignore 忽略
    数据类型 []string

    ## ViewName 操作视图名字
    数据类型 int
    视图名称:year=1, month=2, week=3, day=4 对应年、月、周、日视图类型

    ## ViewTime 操作打开对应时间
    数据类型 string  "2019-10-23T10:00:00"

    ## ADTitleName 删除和查询关键字 可以为空
    数据类型 string

    ## ADStartTime 删除范围和查询 范围操作起始时间 不能缺省
    数据类型 string  "2019-10-23T10:00:00"

    ## ADEndTime 删除范围和查询 范围操作结束时间 不能缺省
    数据类型 string  "2019-10-23T10:00:00"
 */
#include "deepincalendar.h"

#include <QJsonDocument>
#include <QDBusInterface>
#include <QDebug>

#define INVOKE_SCHEDULE_CREATE              ("CREATE")
#define INVOKE_SCHEDULE_VIEW                ("VIEW")
#define INVOKE_SCHEDULE_QUERY               ("QUERY")
#define INVOKE_SCHEDULE_CANCEL              ("CANCEL")

DeepinCalendar::DeepinCalendar(QObject *parent)
    : QObject(parent)
{
}

DeepinCalendar::~DeepinCalendar()
{
}

int DeepinCalendar::createSchedule(const QJsonObject &objCreate)
{
    QVariant retCreate;
    return invoke(INVOKE_SCHEDULE_CREATE, objCreate, retCreate);
}

int DeepinCalendar::viewSchedule(const QJsonObject &objView)
{
    QVariant retView;
    return invoke(INVOKE_SCHEDULE_VIEW, objView, retView);
}

int DeepinCalendar::querySchedule(const QJsonObject &objQuery, QVariant &retQuery)
{
    return invoke(INVOKE_SCHEDULE_QUERY, objQuery, retQuery);
}

int DeepinCalendar::cancelSchedule(const QJsonObject &objCancel)
{
    QVariant vCancelResult;
    return invoke(INVOKE_SCHEDULE_CANCEL, objCancel, vCancelResult);
}

int DeepinCalendar::invoke(const QString &intent, const QJsonObject &objSchedule, QVariant &retInvoke)
{

    QJsonDocument jsonScheduleData;
    jsonScheduleData.setObject(objSchedule);
    QString strSchedule = jsonScheduleData.toJson(QJsonDocument::Compact);

    qWarning() << " intent=" << intent << " json=" << strSchedule;


    QDBusInterface scheduleInterface("com.deepin.Calendar",
                                     "/",
                                     "com.deepin.ExportedInterface");

    QDBusMessage respon = scheduleInterface.call("invoke", intent, strSchedule);
    if (respon.type() == QDBusMessage::ErrorMessage) {
        qCritical() << "Invoke calendar failed: " << respon.errorMessage();
        return -1;
    }

    retInvoke = qvariant_cast<QDBusVariant>(respon.arguments().takeFirst()).variant();

    return 0;
}
