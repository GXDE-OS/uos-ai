#ifndef MEETINGMONITOR_H
#define MEETINGMONITOR_H

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QProcess>
#include <QTimer>
#include <QDir>
#include <QFileInfoList>

namespace uos_ai {
class MeetingMonitor : public QObject
{
    Q_OBJECT
public:
    MeetingMonitor();
    virtual ~MeetingMonitor() {}
signals:
    void sigIsMileMeetingRunning(bool);
public slots:
    void onStartAiMeeting();
private slots:
    void onReadProcDir();
private:
    QString meetingName = "MileMeeting.exe";
    bool m_meetingAssistantRunning = false;  //会议助手是否启动
    QTimer timer;

};
}
#endif // MEETINGMONITOR_H
